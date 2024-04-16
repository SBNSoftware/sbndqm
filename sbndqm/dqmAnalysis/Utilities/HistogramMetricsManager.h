/**
 * @file sbndqm/dqmAnalysis/Utilities/HistogramMetricsManager.h
 * @brief Library to manage metrics with histogram content.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @see sbndqm/dqmAnalysis/Utilities/HistogramMetricsManager.cxx
 * 
 */

#ifndef SBNDQM_DQMANALYSIS_TRIGGER_DETAILS_HISTOGRAMMETRICSMANAGER_H
#define SBNDQM_DQMANALYSIS_TRIGGER_DETAILS_HISTOGRAMMETRICSMANAGER_H

// DQM libraries
#include "sbndqm/dqmAnalysis/Utilities/HistogramMetrics.h"

// framework libraries
#include "fhiclcpp/types/DelegatedParameter.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/ParameterSet.h"

// C/C++ standard libraries
#include <map>
#include <vector>
#include <string>
#include <cstddef> // std::ptrdiff_t


// -----------------------------------------------------------------------------
namespace sbndaq { class HistogramMetricsManager; }
/**
 * @brief Manages multiple metric histograms.
 * 
 * This class is intended as a helper to manage `sbndaq::HistogramMetrics`
 * objects.
 * 
 * It provides:
 *  * access to the configured histograms via a key;
 *  * a (meta)configuration for the metrics and histograms together.
 * 
 * The main point of the manager is the configuration, which is
 * histogram-oriented rather than metric-oriented.
 * 
 * Configuration
 * --------------
 * 
 * The configuration is made of two sections:
 * * `database` (configuration table): includes the parameters common to all
 *    metrics. This is used directly as part of `sbndaq::GenerateMetricConfig()`
 *    configuration.
 * * `histograms` (list of configuration tables), defining each of the
 *    histograms, including:
 *     * the same configuration as `sbndaq::HistogramMetrics` (see
 *       `sbndaq::HistogramMetricsFHiCLconfig`)
 *     * `histName` (string, optional): the name (key) used to access the
 *        histogram in the code; by default, it is `<groupName>_<metricName>`
 *        (with `groupName` and `metricName` the values of the
 *        `sbndaq::HistogramMetrics` parameters) or just `<metricName>` if that
 *        is equal to `<groupName>`.
 * 
 * 
 * Example of configuration:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * HistConfig: {
 *   
 *   database: {
 *     hostname: "icarus-db01.fnal.gov"
 *     streams: [ "slow", "fast", "archive" ]
 *   }
 *   
 *   histograms: [
 *     {
 *       metricName: "BNBspillTime"
 *       title:      "BNB spill time"
 *       lower:      -0.2 # us
 *       upper:      +2.6 # us
 *       bins:      175   # 16 ns bins
 *       unit:        "us"
 *       level:       3
 *     },
 *     {
 *       metricName: "NuMIspillTime"
 *       title:      "NuMI spill time"
 *       lower:      -0.2 # us
 *       upper:     +10.6 # us
 *       bins:      225   # 48 ns bins
 *       unit:        "us"
 *       level:       3
 *     }
 *   ]
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * will configure two histograms:
 *  * one with `metricName` `BNBspillTime` and `groupName` also `BNBspillTime`,
 *    with `175` metrics, associated to the histogram `BNBspillTime` in the code
 *    (because `histName` is not overridden);
 *  * one with `metricName` and `groupName` `NuMIspillTime`, with `225` metrics,
 *    associated to the histogram `NuMIspillTime` in the code.
 * 
 * 
 * Output
 * -------
 * 
 * A group of metrics is created for each histogram, named after the `groupName`
 * configuration parameter of that histogram. If the group name is the same for
 * multiple histograms, they are required to have the same binning or a
 * configuration error will occur.
 * 
 * In each metric group, one histogram is placed as a metric with the name from
 * the `metricName` configuration parameter and as many instance names as the
 * number of bins in the histogram. If there are multiple histograms configured
 * with the same group, they will each have its `metricName` under that group.
 * 
 */
class sbndaq::HistogramMetricsManager {
  
    public:
  
  struct Config {
    
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;
    
    struct HistConfig: sbndaq::HistogramMetricsFHiCLconfig<double> {
      
      fhicl::OptionalAtom<std::string> histName{
        Name{ "histName" },
        Comment{ "key of the histogram being configured" }
        };
      
    }; // HistConfig
    
    
    fhicl::DelegatedParameter database{
      Name{ "database" },
      Comment{ "configuration of the database connection" }
      };
    
    fhicl::Sequence<fhicl::Table<HistConfig>> histograms {
      Name{ "histograms" },
      Comment{ "configuration of all metrics histograms" }
      };
    
    
  }; // Config
  
  /// Constructor from the specified configuration.
  HistogramMetricsManager(Config const& config);
  
  
  /// Returns a configuration used to initialise the metrics, as a list of FHiCL
  /// parameter tables, one for each `sbndaq::GenerateMetricConfig()` call.
  std::vector<fhicl::ParameterSet> const& metricConfiguration() const;
  
  
  // --- BEGIN --- Histogram access --------------------------------------------
  /// @{
  /// @name Histogram access
  
  /// Returns the number of configured metrics histograms.
  std::size_t nHistograms() const noexcept;
  
  /// Returns whether there is a metrics histogram matching the specified `key`.
  bool has(std::string const& key) const;
  
  /**
   * @brief Returns the metrics histogram with the specified `key`.
   * @return the metrics histogram with the specified `key`, `nullptr` if none
   * 
   * The returned metrics histogram object is `const` and can't be modified.
   */
  HistogramMetrics const* get(std::string const& key) const;
  
  /**
   * @brief Returns the metrics histogram with the specified `key`.
   * @return the metrics histogram with the specified `key`, `nullptr` if none
   */
  HistogramMetrics* use(std::string const& key);
  
  /**
   * @brief Returns the metrics histogram with the specified `key`.
   * @return the metrics histogram with the specified `key`
   * @throw std::out_of_range if no histogram exists with `key`
   */
  HistogramMetrics& require(std::string const& key);
  
  
  /// Returns a list of keys of all configured histograms.
  std::vector<std::string> histogramKeys() const;
  
  /// @}
  // --- END ----- Histogram access --------------------------------------------
  
  
    private:
  
  /// Helper record of all the metrics and histogram configurations.
  struct AllConfigs {
    std::vector<fhicl::ParameterSet> metrics;
    std::vector<Config::HistConfig> histograms;
  };
  
  std::map<std::string, HistogramMetrics> fHists; ///< All metrics histograms.
  
  /// All metrics configurations.
  std::vector<fhicl::ParameterSet> fMetricConfig;
  
  
  /// Composes and returns all the configurations for metrics and histograms.
  static AllConfigs buildConfigurations(Config const& config);
  
}; // sbndaq::HistogramMetricsManager


// -----------------------------------------------------------------------------

#endif // SBNDQM_DQMANALYSIS_TRIGGER_DETAILS_HISTOGRAMMETRICSMANAGER_H
