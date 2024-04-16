/**
 * @file sbndqm/dqmAnalysis/Utilities/HistogramMetrics.h
 * @brief Library to manage metrics with histogram content.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @see sbndqm/dqmAnalysis/Utilities/HistogramMetrics.cxx
 * 
 */

#ifndef SBNDQM_DQMANALYSIS_TRIGGER_DETAILS_HISTOGRAMMETRICS_H
#define SBNDQM_DQMANALYSIS_TRIGGER_DETAILS_HISTOGRAMMETRICS_H

// framework libraries
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/Atom.h"

// ROOT libraries
#include "TH1.h"

// C/C++ standard libraries
#include <sstream>
#include <vector>
#include <string>
#include <cstddef> // std::ptrdiff_t


// -----------------------------------------------------------------------------
namespace sbndaq {
  class HistogramMetrics;
  template <typename T> struct HistogramMetricsFHiCLconfig;
  HistogramMetrics makeHistogramMetrics
    (HistogramMetricsFHiCLconfig<double> const& config);
} // namespace sbndaq

/**
 * @brief Metrics manager with a histogram-like interface.
 * 
 * This class is bound to a group of metrics with an index from `0` to _N_,
 * which it treats like bins of an histogram.
 * 
 * Metrics must be configured by the first time the histogram is `Fill()`ed or
 * `Reset()`, and it is expected that they are `Reset()` immediately after
 * the configuration of the relevant metrics.
 * 
 * Example of configuration:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 *   TriggerGateHistogram: {
 *     groupName:   "TriggerGateHist"
 *     metricsName: "BNBspillTime"
 *     lower:       -0.2 # us
 *     upper:       +2.6 # us
 *     bins:       175   # 16 ns bins
 *     level:        3
 *   } # TriggerGateHistogram
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 * For a helper able to configure the metrics and the histogram together, see
 * `sbndaq::HistogramMetricsManager`.
 */
class sbndaq::HistogramMetrics {
  
    public:
  
  using Value_t = double; ///< Type of the value being binned.
  using Count_t = double; ///< Type of histogram count (the metric content).
  
  using BinNumber_t = std::ptrdiff_t; ///< Type of bin index.
  
  /**
   * @brief Constructor: sets the binning and the metrics.
   * @param groupName name of the group the metrics are in
   * @param metricsName the name of the metrics managed by this object
   * @param title title of the metrics and their histogram
   * @param nBins the number of metrics (from `0` to `nBins` excluded)
   * @param lower lower bound of the histogram axis
   * @param upper upper bound of the histogram axis
   * @param units (default: empty) units of the metrics value
   * @param level (default: `3`) metrics level
   * @param logCategory (default: `"HistogramMetrics"`) message stream name
   */
  HistogramMetrics(
    std::string groupName, std::string metricsName,
    std::string title,
    unsigned int nBins, Value_t lower, Value_t upper,
    std::string units = "",
    int level = 3, std::string logCategory = "HistogramMetrics"
    );
  
  // --- BEGIN --- Binning query -----------------------------------------------
  /**
   * @name Binning query
   * 
   * Bin numbers also match the metric instance name (conversion should be
   * performed with `binName()` though).
   * 
   * Bins start from `0`.
   */
  /// @{
  
  /// Returns the number of bins (and metrics) in this histograms.
  std::size_t NBins() const;
  
  /// Returns whether this histogram has the specified `bin`.
  bool HasBin(BinNumber_t bin) const;
  
  /// Returns the index of the bin containing `value` (check with `HasBin()`).
  BinNumber_t FindBin(Value_t value) const;
  
  
  /// Returns the name of the `bin` (and of its instance in the metric).
  std::string const& binName(BinNumber_t bin) const;
  
  /// @}
  // --- END ----- Binning query -----------------------------------------------
  
  /// Increments by `weight` the metric corresponding to the specified `value`.
  void Fill(Value_t value, Count_t weight = Count_t{ 1 });
  
  /// Sets all bin content and metrics to `0`.
  void Reset();
  
  
  /// Returns the lower edge of `bins` from `lower` to `upper`.
  static std::vector<double> makeBinLowerEdges
    (double lower, double upper, unsigned int bins);

  /// Returns the lower edge of `bins` from `lower` to `upper`, as strings.
  static std::vector<std::string> makeBinLabels
    (double lower, double upper, unsigned int bins);

  /// Returns a collection of strings, each representing a value of `coll`.
  template <typename Coll>
  static std::vector<std::string> seqToStringSeq(Coll const& seq);
  
    private:
  
  std::string fGroupName;   ///< Name of the group of the managed metrics.
  std::string fMetricName;  ///< Name of the managed metrics.
  int fLevel;               ///< Level for sending metric values.
  
  // I am using a ROOT histogram for convenience, plus the future possibility to
  // save it into a ROOT file (art::TFileService) if desired.
  TH1F fHist;               ///< Bin content.
  
  std::string fLogCategory; ///< Message facility stream name.
  
  std::vector<std::string> fBinNames; ///< Name of each of the bins.
  
  /// Increments the `bin` by an `amount` and returns the new bin content.
  Count_t addToBin(BinNumber_t bin, Count_t amount);
  
  /// Converts the `bin` number into the implementation-specific bin number.
  static int toImplBin(BinNumber_t bin);
  
  /// Converts the implementation bin number `implBin` into a bin number.
  static BinNumber_t fromImplBin(int implBin);
  
  /// Creates a list of bin names out of a ROOT histogram.
  static std::vector<std::string> makeBinLabels(TH1& hist);
  
}; // sbndaq::HistogramMetrics


// -----------------------------------------------------------------------------
/**
 * @brief FHiCL configuration for a `HistogramMetrics` object.
 * @tparam T type of the data represented by the metric histogram
 */
template <typename T>
struct sbndaq::HistogramMetricsFHiCLconfig {
  
  using Data_t = T; ///< Type of data in the metrics.
  
  using Name = fhicl::Name;
  using Comment = fhicl::Comment;
  
  fhicl::OptionalAtom<std::string> groupName {
    Name{ "groupName" },
    Comment{ "name of the metrics group the histogram belongs to" }
    };
  
  fhicl::Atom<std::string> metricsName {
    Name{ "metricsName" },
    Comment{ "name of the metrics the histogram is managing" }
    };
  
  fhicl::Atom<std::string> title {
    Name{ "title" },
    Comment{ "title of the metrics (for plots)" },
    ""
    };
  
  fhicl::Atom<std::string> units {
    Name{ "units" },
    Comment{ "units of the metrics (for plots)" },
    ""
    };
  
  fhicl::Atom<unsigned int> bins {
    Name{ "bins" },
    Comment{ "number of bins in the histogram (and of metrics in the group)" }
    };
  
  fhicl::Atom<Data_t> lower {
    Name{ "lower" },
    Comment{ "lower boundary of the histogram axis" }
    };
  
  fhicl::Atom<Data_t> upper {
    Name{ "upper" },
    Comment{ "upper boundary of the histogram axis" }
    };
  
  fhicl::Atom<int> level {
    Name{ "level" },
    Comment{ "metrics level" },
    3 // arbitrary
    };
  
}; // sbndaq::HistogramMetricsFHiCLconfig<>


// -----------------------------------------------------------------------------
// ---  Template implementation
// -----------------------------------------------------------------------------
template <typename Coll>
std::vector<std::string> sbndaq::HistogramMetrics::seqToStringSeq
  (Coll const& seq)
{
  using std::to_string;
  std::vector<std::string> sseq;
  sseq.reserve(size(seq));
  std::ostringstream sstr;
  for (auto const& v: seq) {
    sstr.str("");
    sstr << v;
    sseq.push_back(sstr.str());
  }
  return sseq;
} // sbndaq::HistogramMetrics::seqToStringSeq()


// -----------------------------------------------------------------------------

#endif // SBNDQM_DQMANALYSIS_TRIGGER_DETAILS_HISTOGRAMMETRICS_H
