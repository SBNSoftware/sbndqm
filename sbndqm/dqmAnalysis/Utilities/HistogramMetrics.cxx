/**
 * @file sbndqm/dqmAnalysis/Utilities/HistogramMetrics.cxx
 * @brief Library to manage metrics with histogram content.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @see sbndqm/dqmAnalysis/Utilities/HistogramMetrics.h
 * 
 * This is an experimental library.
 * 
 */

// library header
#include "sbndqm/dqmAnalysis/Utilities/HistogramMetrics.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// framework libraries
#include "sbndaq-online/helpers/SBNMetricManager.h" // sbndaq::sendMetric()

// C/C++ standard libraries
#include <iomanip> // std::setw()
#include <sstream>
#include <utility> // std::move()
#include <cmath> // std::floor()


// -----------------------------------------------------------------------------
// ---  sbndaq::HistogramMetrics
// -----------------------------------------------------------------------------
sbndaq::HistogramMetrics::HistogramMetrics(
  std::string groupName, std::string metricsName,
  std::string title,
  unsigned int nBins, Value_t lower, Value_t upper,
  std::string units /* = "" */,
  int level /* = 3 */, std::string logCategory /* = "HistogramMetrics" */
)
  : fGroupName { std::move(groupName) }
  , fMetricName{ std::move(metricsName) }
  , fLevel     { level }
  , fHist{
    ("H" + fMetricName + "_" + fGroupName).c_str(),
    ((title.empty()? ("Metric: " + fMetricName): title) + ";;" + units).c_str(),
    static_cast<int>(nBins), lower, upper
    }
  , fLogCategory{ std::move(logCategory) }
  , fBinNames{ makeBinLabels(fHist) }
{
  fHist.SetDirectory(nullptr);
  // no reset here because we don't require the metrics to be configured yet
}


// -----------------------------------------------------------------------------
std::size_t sbndaq::HistogramMetrics::NBins() const {
  return static_cast<std::size_t>(fHist.GetNbinsX());
}


// -----------------------------------------------------------------------------
bool sbndaq::HistogramMetrics::HasBin(BinNumber_t bin) const {
  return (bin >= 0) && (bin < static_cast<BinNumber_t>(NBins()));
}


// -----------------------------------------------------------------------------
auto sbndaq::HistogramMetrics::FindBin(Value_t value) const -> BinNumber_t {
  return fromImplBin(fHist.FindFixBin(value));
}


// -----------------------------------------------------------------------------
void sbndaq::HistogramMetrics::Fill
  (Value_t value, Count_t weight /* = Count_t{ 1 } */)
{
  
  std::ptrdiff_t const bin = FindBin(value);
  if (!HasBin(bin)) return; // overflow, underflow, whatever: we skip it
  
  Count_t const binCount = addToBin(bin, weight);
  
  std::string const& instanceName = binName(bin);
  mf::LogTrace{ fLogCategory }
    << "sendMetric(" << fGroupName << ", " << instanceName
    << ", " << fMetricName << ", " << binCount << ", " << fLevel
    << ", " << static_cast<int>(artdaq::MetricMode::LastPoint) << ")";
  sbndaq::sendMetric(
    fGroupName, instanceName, fMetricName, binCount, fLevel,
    artdaq::MetricMode::LastPoint
    );
  
} // sbndaq::HistogramMetrics::Fill()


// -----------------------------------------------------------------------------
void sbndaq::HistogramMetrics::Reset() {
  
  // clear the local bin content
  int const nImplBins = fHist.GetNbinsX() + 1;
  for (int implBin = -1; implBin <= nImplBins; ++implBin)
    fHist.SetBinContent(implBin, 0.0);
  fHist.ResetStats();
  
  // reset the metrics
  BinNumber_t const nBins = static_cast<BinNumber_t>(NBins());
  for (BinNumber_t bin = 0; bin < nBins; ++bin) {
    std::string const& instanceName = binName(bin);
    mf::LogTrace{ fLogCategory }
      << "sendMetric(" << fGroupName << ", " << instanceName
      << ", " << fMetricName << ", " << Count_t{ 0 } << ", " << fLevel
      << ", " << static_cast<int>(artdaq::MetricMode::LastPoint) << ")";
    sbndaq::sendMetric(
      fGroupName, instanceName, fMetricName, Count_t{ 0 }, fLevel,
      artdaq::MetricMode::LastPoint
      );
  } // for
  
} // sbndaq::HistogramMetrics::Reset()


// -----------------------------------------------------------------------------
std::string const& sbndaq::HistogramMetrics::binName(BinNumber_t bin) const {
  return fBinNames[bin];
}


// -----------------------------------------------------------------------------
std::vector<double> sbndaq::HistogramMetrics::makeBinLowerEdges
  (double lower, double upper, unsigned int bins)
{
  // returns the left (lower) edge of each bin
  double const width = (upper - lower) / bins;
  std::vector<double> edges;
  edges.reserve(bins);
  // using product is slower but more precise (neither should actually matter)
  for (unsigned int iBin = 0; iBin < bins; ++iBin)
    edges.push_back(lower + iBin * width);
  return edges;
} // sbndaq::HistogramMetricsManager::makeBinLowerEdges()


// -----------------------------------------------------------------------------
std::vector<std::string> sbndaq::HistogramMetrics::makeBinLabels
  (double lower, double upper, unsigned int bins)
{
  /*
   * There is some issue with instance names in the form of a real number
   * (maybe sorting?), probably just issues of presentation in minargon.
   * The interface seems to rework the instance names replaces spaces with "_",
   * removing dots altogether and who knows what else, and then sorts the
   * metrics lexicographically according to their instance name.
   * This means that whatever we do with the labels here, we need to guarantee
   * that they are lexicographically sorted (including the effect of the
   * replacement above). The current workaround is to drive the sorting by
   * prepending the padded bin number (`[paddedbin] `) to the actual value
   * of the bin. I am not sure whether there is a (reasonable) format that
   * whose lexicographical order matches the numeric one
   * (unfortunately, '-' character is "larger" than both space and '+').
   * The only exception to this appears to be if the instance name can be
   * converted to an integer (not clear if it needs to be a positive one),
   * in which case the order seems to be correct.
   */
#if 1
  
  std::vector<std::string> labels;
  labels.reserve(bins);
  
  auto isInteger = [](double v)
    { return v? ((v - std::floor(v))/(v + std::floor(v)) < 1e-5): true; };
  
  double const binWidth = (upper - lower) / bins;
  if (isInteger(binWidth) && isInteger(lower)) {
    int const intBinWidth = static_cast<int>(binWidth);
    int bin = static_cast<int>(lower);
    for (unsigned int iBin = 0; iBin < bins; ++iBin, bin += intBinWidth)
      labels.push_back(std::to_string(bin));
  }
  else {
    unsigned int const padding = std::to_string(bins - 1).length();
    
    std::vector<double> const edges = makeBinLowerEdges(lower, upper, bins);
    std::ostringstream sstr;
    sstr.fill('0');
    for (unsigned int iBin = 0; iBin < bins; ++iBin) {
      sstr.str("");
      sstr << "[" << std::setw(padding) << iBin << "] " << edges[iBin];
      labels.push_back(sstr.str());
    }
  }
  return labels;
#else
  return seqToStringSeq(makeBinLowerEdges(lower, upper, bins));
#endif
}


// -----------------------------------------------------------------------------
auto sbndaq::HistogramMetrics::addToBin(std::ptrdiff_t bin, Count_t amount)
  -> Count_t
{
  int const implBin = toImplBin(bin); // ROOT convention on bin number...
  fHist.AddBinContent(implBin, amount);
  return static_cast<Count_t>(fHist.GetBinContent(implBin));
} // sbndaq::HistogramMetrics::addToBin()


// -----------------------------------------------------------------------------
int sbndaq::HistogramMetrics::toImplBin(BinNumber_t bin) {
  // following ROOT convention, regular bins are from 1 to N included:
  return static_cast<int>(bin + 1);
}


// -----------------------------------------------------------------------------
auto sbndaq::HistogramMetrics::fromImplBin(int implBin) -> BinNumber_t {
  // following ROOT convention, regular bins are from 1 to N included:
  return static_cast<BinNumber_t>(implBin - 1);
}


// -----------------------------------------------------------------------------
std::vector<std::string> sbndaq::HistogramMetrics::makeBinLabels(TH1& hist) {
  int const nBins = hist.GetNbinsX();
  return
    makeBinLabels(hist.GetBinLowEdge(1), hist.GetBinLowEdge(nBins+1), nBins);
} // sbndaq::HistogramMetrics::makeBinLabels()


// -----------------------------------------------------------------------------
// ---  sbndaq::makeHistogramMetrics()
// -----------------------------------------------------------------------------
sbndaq::HistogramMetrics sbndaq::makeHistogramMetrics
  (sbndaq::HistogramMetricsFHiCLconfig<double> const& config)
{
  return sbndaq::HistogramMetrics{
      config.groupName().value_or(config.metricsName()), config.metricsName()
    , config.title()
    , config.bins(), config.lower(), config.upper()
    , config.units()
    , config.level()
    };
} // sbndaq::TriggerMonitorWithDecoder::makeHistogramMetrics()


// -----------------------------------------------------------------------------
