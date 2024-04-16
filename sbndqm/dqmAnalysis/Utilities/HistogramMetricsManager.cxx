/**
 * @file sbndqm/dqmAnalysis/Utilities/HistogramMetricsManager.cxx
 * @brief Library to manage metrics with histogram content (implementation).
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @see sbndqm/dqmAnalysis/Utilities/HistogramMetricsManager.h
 */

// this library
#include "sbndqm/dqmAnalysis/Utilities/HistogramMetricsManager.h"

// SBN libraries
#include "sbndaq-online/helpers/MetricConfig.h"

// framework libraries
#include "cetlib_except/exception.h"

// C++ standard libraries
#include <utility> // std::move()
#include <optional>
#include <cassert>


// -----------------------------------------------------------------------------
namespace {
  
  /// Returns a pointer to the value of `key` in `map`, or `nullptr` if none.
  template <typename Map, typename Key>
  auto getPtr(Map& map, Key const& key) {
    auto const it = map.find(key);
    return (it == map.end())? nullptr: &(it->second);
  }
  
  /// Sets the content of the FHiCL "typed" object to the specified `value`.
  template <typename FCL>
  void setFHiCLvalue(FCL& obj, std::string const& value) {
    // why is it so fucking hard to set a value??
    // the only way seems to be via `set_value()`, which requires a whole
    // parameter set with the full key of `obj` and the new value assigned to it
    // so we need to `ParameterSet::make()` such parameter set, which would
    // require escaping the value, or create the parameter set from scratch
    // adding level after level, and each level must be the correct type (that
    // is usually a table but it might also be a sequence of tables with an
    // index). Either way, `make()` or build from scratch, face the fact that
    // the first element of `key()` may be garbage (or, more precisely,
    // a key with a name that is not a valid FHiCL key), so we need to trim it,
    // and then tell `set_value()` not to look for that part.
    std::string key = obj.key();
    //bool trim = false; // probably going to be true...
    if (auto const pos = key.find('.'); pos != std::string::npos) {
      key.erase(0, pos+1);
      //trim = true;
    }
    auto paramValue = fhicl::ParameterSet::make(key + ": '" + value + "'");
    obj.set_value(paramValue); //, trim);
  }
  
  /// Returns a FHiCL string representation of a sequence. Not robust at all.
  std::string toFHiCL(std::vector<std::string> const& seq) {
    std::string cfg = "[";
    if (!seq.empty()) {
      auto it = seq.begin();
      auto const send = seq.end();
      cfg += " \"" + *it + "\"";
      while (++it != send) cfg += ", \"" + *it + "\"";
    }
    cfg += " ]";
    return cfg;
  } // toFHiCL()
  
  template <typename T>
  constexpr T absDiff(T a, T b) { return (a > b)? (a - b): (b - a); }
  
  /// Returns whether `a` and `b` have a close enough value.
  template <typename T>
  constexpr bool closeValue(T a, T b, T tol) {
    constexpr T zero{};
    if ((a == zero) || (b == zero) || (a + b < tol)) {
      return absDiff(a, b) < tol;
    }
    else {
      return (absDiff(a, b) / (a + b)) <= tol;
    }
  } // closeValue()
  
  /// Returns whether all the elements in `A` and `B` have the same value.
  template <typename CollA, typename CollB, typename T>
  bool closeValues(CollA const& A, CollB const& B, T tol) {
    auto const close = [tol](auto a, auto b){ return closeValue(a, b, tol); };
    return std::equal(begin(A), end(A), begin(B), close);
  }
  
} // local namespace


// -----------------------------------------------------------------------------
sbndaq::HistogramMetricsManager::HistogramMetricsManager(Config const& config) {
  
  AllConfigs configs = buildConfigurations(config);
  
  // first configure all the needed metrics
  for (fhicl::ParameterSet& config: configs.metrics) {
    sbndaq::GenerateMetricConfig(config);
    fMetricConfig.push_back(std::move(config));
  }
  
  // then create all the required histograms
  for (Config::HistConfig const& config: configs.histograms) {
    
    assert(config.histName());
    std::string key = *config.histName();
    
    auto hint = fHists.lower_bound(key);
    if ((hint != fHists.end()) && (hint->first == key)) {
      throw cet::exception{ "HistogramMetricsManager" }
        << "Metrics histogram '" << key << "' configured more than once.\n";
    }
    
    auto hist = makeHistogramMetrics(config);
    hist.Reset();
    fHists.emplace_hint(hint, std::move(key), std::move(hist));
    
  } // for histogram configurations
  
  
} // sbndaq::HistogramMetricsManager::HistogramMetricsManager()


// -----------------------------------------------------------------------------
std::vector<fhicl::ParameterSet> const&
sbndaq::HistogramMetricsManager::metricConfiguration() const
  { return fMetricConfig; }


// -----------------------------------------------------------------------------
std::size_t sbndaq::HistogramMetricsManager::nHistograms() const noexcept
  { return fHists.size(); }


// -----------------------------------------------------------------------------
bool sbndaq::HistogramMetricsManager::has(std::string const& key) const
  { return fHists.count(key) > 0; }


// -----------------------------------------------------------------------------
auto sbndaq::HistogramMetricsManager::get(std::string const& key) const
  -> HistogramMetrics const*
{
  auto const it = fHists.find(key);
  return (it == fHists.end())? nullptr: &(it->second);
}


// -----------------------------------------------------------------------------
auto sbndaq::HistogramMetricsManager::use(std::string const& key)
  -> HistogramMetrics*
{
  auto const it = fHists.find(key);
  return (it == fHists.end())? nullptr: &(it->second);
}


// -----------------------------------------------------------------------------
auto sbndaq::HistogramMetricsManager::require(std::string const& key)
  -> HistogramMetrics&
{
  if (HistogramMetrics* hist = use(key)) return *hist;
  cet::exception e{ "HistogramMetricsManager" };
  e << "No metric histogram with key '" + key + "'.\nAvailable:";
  for (auto const& pair: fHists) e << " '" << pair.first << "'";
  throw e << ".\n";
}


// -----------------------------------------------------------------------------
std::vector<std::string> sbndaq::HistogramMetricsManager::histogramKeys() const
{
  std::vector<std::string> keys;
  for (auto const& pair: fHists) keys.push_back(pair.first);
  return keys;
} // sbndaq::HistogramMetricsManager::histogramKeys()


// -----------------------------------------------------------------------------
auto sbndaq::HistogramMetricsManager::buildConfigurations(Config const& config)
  -> AllConfigs
{
  /*
   * We create one configuration per group (no metric has multiple groups in
   * this configuration), and that configuration may include one or more
   * metrics.
   *
   * Example of metrics config:
   *     
   *     metric_config: {
   *     
   *       hostname: "icarus-db01.fnal.gov"
   *     
   *       streams: [ "slow", "fast", "archive" ]
   *       
   *       groups: {
   *         
   *         groupName:  [ [ 0, 500 ] ]
   *         
   *       }
   *       
   *       metrics: {
   *     
   *         metricsName:{
   *           units: "us"
   *           title: "BNB trigger time minus beam gate opening time"
   *           display_range: [ -0.5, 3.0 ]
   *         }
   *         
   *       }
   *     } # metric_config
   *     
   */
  std::vector<fhicl::ParameterSet> metrics;
  std::vector<Config::HistConfig> histograms;
  
  fhicl::ParameterSet const dbConfig
    = config.database.get<fhicl::ParameterSet>();
  
  std::map<std::string, fhicl::ParameterSet> groupConfigs;
  
  for (Config::HistConfig histConfig: config.histograms()) {
    
    std::string const& metricsName = histConfig.metricsName();
    
    if (!histConfig.groupName())
      setFHiCLvalue(histConfig.groupName, metricsName);
    std::string const groupName = *(histConfig.groupName());
    
    if (!histConfig.histName()) {
      setFHiCLvalue(histConfig.histName,
        (metricsName == groupName)? metricsName: groupName + "_" + metricsName);
    }
    
    unsigned int nBins = histConfig.bins();
    if (nBins < 1) {
      throw cet::exception{ "HistogramMetricsManager" }
        << "Invalid number of bins for histogram '"
        << groupName << "/" << metricsName << "': " << nBins << "\n";
    }
    
    
    fhicl::ParameterSet* groupConfig = getPtr(groupConfigs, groupName);
    fhicl::ParameterSet metricsConfig;
    
    if (groupConfig) {
      
      std::vector<std::string> const bins = HistogramMetrics::makeBinLabels
        (histConfig.lower(), histConfig.upper(), nBins);
      
      std::vector<std::string> oldBins;
      try {
        oldBins
          = groupConfig->get<std::vector<std::string>>("groups." + groupName);
      }
      catch(cet::exception const& e) {
        throw cet::exception{ "HistogramMetricsManager", "", e }
          << "Internal error attempting to read 'groups." << groupName
          << "' from:\n"
          << groupConfig->to_indented_string(1) << "\n";
      }
      
      if (bins != oldBins) {
      // if (!closeValues(bins, oldBins, 1e-5)) {
      //   auto binWidth = [](auto const& edges)
      //     { return (edges.back() - edges.front()) / (edges.size() - 1); };
        throw cet::exception{ "HistogramMetricsManager" }
          << "Configuration error attempting to read 'groups." << groupName
          << "' from:\n"
          << groupConfig->to_indented_string(1) << "\n"
          << "\nInconsistent binning:"
          << "\n - before: " << oldBins.size() << " bins " // << binWidth(oldBins)
            << " wide: " << toFHiCL(HistogramMetrics::seqToStringSeq(oldBins))
          << "\n - then:   " << bins.size() << " bins " // << binWidth(bins)
            << " wide: " << toFHiCL(HistogramMetrics::seqToStringSeq(bins))
          << "\n";
      }
      
      // will be overwritten later
      metricsConfig = groupConfig->get<fhicl::ParameterSet>("metrics");
      
    }
    else {
      
      // create the new group with empty metrics table
      groupConfig = &(groupConfigs[groupName]);
      
      *groupConfig = dbConfig;
      
      std::vector<std::string> const bins = HistogramMetrics::makeBinLabels
        (histConfig.lower(), histConfig.upper(), nBins);
      groupConfig->put
        ("groups", fhicl::ParameterSet::make(groupName + ": " + toFHiCL(bins)));
      
    }
    
    // create the new metric configuration and add it groupConfig
    if (groupConfig->has_key("metrics." + metricsName)) {
      throw cet::exception{ "HistogramMetricsManager" }
        << "Configuration error: multiple '" << metricsName 
        << "' metrics configured within group name '" << groupName << "'\n";
    }
    
    fhicl::ParameterSet metricsCfg;
    metricsCfg.put("units", histConfig.units());
    metricsCfg.put("title", histConfig.title());
    
    metricsConfig.put(metricsName, metricsCfg);
    
    groupConfig->put_or_replace("metrics", metricsConfig);
    
    // create the histogram configuration
    histograms.push_back(histConfig);
    
  } // for histograms
  
  for (auto& groupAndConfig: groupConfigs)
    metrics.push_back(std::move(groupAndConfig.second));
  
  return { std::move(metrics), std::move(histograms) };
} // sbndaq::HistogramMetricsManager::buildConfigurations()


// -----------------------------------------------------------------------------
