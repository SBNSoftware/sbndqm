/**
 * @file TriggerMonitorWithDecoder_module.cc
 * @brief Real-time trigger quality monitor.
 * @authors Andrea Scarpelli <ascarpell@bnl.gov>,
 *          Donatella Torretta <torretta@fnal.gov>,
 *          Gianluca Petrillo <petrillo@slac.stanford.edu>
 */

// LArSoft/SBN libraries
#include "sbndqm/dqmAnalysis/Utilities/HistogramMetricsManager.h"
#include "sbndqm/dqmAnalysis/Utilities/HistogramMetrics.h"
#include "sbnobj/Common/Trigger/ExtraTriggerInfo.h"
#include "sbnobj/Common/Trigger/BeamBits.h"
#include "larcorealg/CoreUtils/enumerate.h"
#include "larcorealg/CoreUtils/counter.h"

// framework libraries
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/Event.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"

// C/C++ standard libraries
#include <array>
#include <bitset>
#include <string>
#include <iomanip> // std::dec, std::hex
#include <cstddef> // std::uint64_t, ...


namespace sbndaq { class TriggerMonitorWithDecoder; };
/**
 * @brief Sends metrics from decoded trigger information.
 * 
 * This data quality monitor module uses the already decoded trigger information
 * to send metrics information about the trigger to the data quality monitoring
 * database.
 * 
 * The configuration of the module defines the name of the metrics and groups
 * being filled.
 * 
 * 
 * Configuration
 * --------------
 * 
 * * `MetricsHistograms` (configuration table): configuration of the manager
 *    of the metrics histograms; see also `sbndaq::HistogramMetricsManager` for
 *    the details.
 *     * `database` (configuration table): configuration of connection to
 *        database host and streams
 *     * `histograms` (list of configuration tables): each entry defines the
 *        configuration for an histogram; the following histograms are required:
 *         * `BNBspillTime`, `BNBenableTime`
 *         * `OffbeamBNBspillTime`, `OffbeamBNBenableTime`
 *         * `NuMIspillTime`, `NuMIenableTime`
 *         * `OffbeamNuMIspillTime`, `OffbeamNuMIenableTime`
 *         * `CalibspillTime`, `CalibenableTime`
 *         * `LVDSbitsEE`, `LVDSbitsEW`, `LVDSbitsWE`, `LVDSbitsWW`
 * 
 * 
 * Input
 * ------
 * 
 * * `sbn::ExtraTriggerInfo` (tag from `TriggerLabel`): decoded trigger
 *   information
 * 
 * 
 * Output
 * -------
 * 
 * Metrics as in the module configuration.
 * 
 * * metrics histograms configuration determines the name of the metrics and
 *   their group for each of the histograms as documented above
 *    * `XXXspillTime`: the time of the trigger with respect to the beam gate
 *      opening
 *    * `XXXenableTime`: the time of the trigger with respect to the PMT enable
 *      gate opening
 *    * `LVDSbitsXX`: the number of times each of the bits (on abscissa) are
 *      found set at trigger time. This is not gate-specific.
 *      Adder bits are included if available.
 * 
 * 
 * How to add an histogram
 * ------------------------
 * 
 * From the point of view of filling metrics, which is the task of this module,
 * to add a new histogram the following steps are required:
 * 1. Add in the configuration a new metrics histogram; this is an entry in
 *    the `Histograms` configuration.
 *     * The metric + group name needs to be unique (you may need to check with
 *       the database, but the easiest way is to have the group name or the
 *       metric name start with a tag like `Trigger`)
 *     * The histogram name (`histName`) is made up from the group and metrics
 *       names (`groupsName` and `metricsName` respectively), unless it is
 *       explicitly specified. When not specified, it is assigned either the
 *       concatenation `<groupName>_<metricsName>` or, if these two parameters
 *       are the same, simply `<metricsName>`. Either way, _the histogram name
 *       is the only information needed when filling the metrics in the code_.
 *     * If the new histogram shares the binning with others, they can share a
 *       metrics group: in that case, `groupName` needs to be specified to be
 *       the same for all of them. Note that in that case the binning
 *       configuration still needs to be duplicated on each of the histograms:
 *       it's not enough for only the first of the histograms in the group to
 *       specify the binning.
 *     * The number of bins, lower and upper limit of the axis must be provided.
 *       A title is recommended, while the units are definitely optional.
 * 2. Change the module code to fill the metrics histogram. Assuming that the
 *    name of the histogram is `"histName"` and the metrics value to be added is
 *    in the `value` variable, the line to add is:
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 *    fHists.require("histName").Fill(value);
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *    which will increment by `1` the bin of the `histName` metrics
 *    corresponding to `value`. An additional parameter can be added to use a
 *    "weight" other than `1`.
 * 
 */
class sbndaq::TriggerMonitorWithDecoder: public art::EDAnalyzer {

    public:
  
  struct Config {
    using Name = fhicl::Name;
    using Comment = fhicl::Comment;
    
    fhicl::Atom<art::InputTag> TriggerLabel{
      Name{ "TriggerLabel" },
      Comment{ "decoded trigger data product tag" },
      "daqTrigger"
      };
    
    fhicl::Atom<std::string> LogCategory{
      Name{ "LogCategory" },
      Comment{ "name of the host with the REDIS database" },
      "TriggerMonitor"
      };
    
    fhicl::Table<sbndaq::HistogramMetricsManager::Config> MetricsHistograms {
      Name{ "MetricsHistograms" },
      Comment{ "configuration of the metrics histograms" }
      };
    
  }; // struct Config
  
  using Parameters = art::EDAnalyzer::Table<Config>;
  

  explicit TriggerMonitorWithDecoder(Parameters const& params);

  void beginRun(art::Run const& run) override;
  
  void analyze(art::Event const & evt) override;
  
    private:
  
  /// Data for a gate type (BNB, NuMI etc.)
  struct GateCounters_t {
    
    /// Highest accepted value for the spill time [ns]
    double SpillTimeMaxAlarm;
    
    /// Number of spill times above the accepted interval.
    unsigned int nSpillTimeMaxAlarms = 0;
    
    /// Highest accepted value for the enable time [ns]
    double EnableTimeMaxAlarm;
    
    /// Number of enable times above the accepted interval.
    unsigned int nEnableTimeMaxAlarms = 0;
    
    std::string name; ///< Name of the gate.
    
  }; // GateCounters_t
  
  /// Magic value for invalid run number.
  static constexpr art::RunNumber_t InvalidRun = 0;
  
  // --- BEGIN -- Configuration parameters -------------------------------------
  
  art::InputTag const fTriggerTag;

  std::string const fLogCategory;
  
  // --- END ---- Configuration parameters -------------------------------------
  
  // --- BEGIN --- Cached counters ---------------------------------------------
  
  art::RunNumber_t fLastRun = InvalidRun; ///< The last run seen by this module.
  
  /// Gate counting information for all gates (index: `sbn::triggerSource`).
  std::array<GateCounters_t, value(sbn::triggerSource::NBits)> fGateCounters;
  
  sbndaq::HistogramMetricsManager fHists; ///< All histograms.
  
  // --- END ----- Cached counters ---------------------------------------------
  
  
  // --- BEGIN -- settings to match to configuration ---------------------
  
  static std::vector<std::string> const RequiredHistograms;
  
  // --- END ---- settings to match to configuration ---------------------

  void resetCounters();
  
  /// Manages the gate timestamps and sends their metrics.
  void monitorTriggerGate(sbn::ExtraTriggerInfo const& trigInfo);
  
  /// Manages the LVDS timestamps and sends their metrics.
  void monitorLVDSbits(sbn::ExtraTriggerInfo const& trigInfo);
  
  /**
   * @brief Sends metrics of a single LVDS connector.
   * @param metricsName name of the metric representing the connector
   * @param connectorBits the bits on the connector
   * @param offset (default: `0`) bit _N_ will be sent to metric _N_ + `offset`
   */
  void sendConnectorBitMetrics(
    std::string const& metricsName, std::uint64_t connectorBits,
    std::ptrdiff_t offset = 0
    );
  
}; // class sbndaq::TriggerMonitorWithDecoder


// -----------------------------------------------------------------------------
// ---  implementation
// -----------------------------------------------------------------------------
std::vector<std::string> const
sbndaq::TriggerMonitorWithDecoder::RequiredHistograms{
  "BNBspillTime",         "BNBenableTime",
  "OffbeamBNBspillTime",  "OffbeamBNBenableTime",
  "NuMIspillTime",        "NuMIenableTime",
  "OffbeamNuMIspillTime", "OffbeamNuMIenableTime",
  "CalibspillTime",       "CalibenableTime",
  
  "LVDSbitsEE", "LVDSbitsEW",
  "LVDSbitsWE", "LVDSbitsWW"
};


// -----------------------------------------------------------------------------
sbndaq::TriggerMonitorWithDecoder::TriggerMonitorWithDecoder
  (Parameters const& params)
  : art::EDAnalyzer{ params }
    // configuration parameters
  , fTriggerTag    { params().TriggerLabel()  }
  , fLogCategory   { params().LogCategory()   }
    // cached
  , fHists         { params().MetricsHistograms() }
{
  //
  // configuration checks
  //
  { // print the metrics configuration being used for the histograms
    mf::LogDebug log{ fLogCategory };
    log
      << "Configuration of " << fHists.nHistograms() << " metrics histograms:";
    for (std::string const& key: fHists.histogramKeys())
      log << " '" << key << "'";
    log << ":";
    for (auto const& [ iHist, pset ]
      : util::enumerate(fHists.metricConfiguration())
    ) {
      log << "\n [" << iHist << "] {\n" << pset.to_indented_string(2)
        << "\n     }";
    }
  }
  
  // check that all the required histograms are configured
  std::vector<std::string> missing;
  for (std::string const& reqName: RequiredHistograms)
    if (!fHists.has(reqName)) missing.push_back(reqName);
  if (!missing.empty()) {
    art::Exception e{ art::errors::Configuration };
    e << missing.size() << "/" << RequiredHistograms.size()
      << " required histograms missing a configuration in '"
      << params().MetricsHistograms.name() << "' configuration parameter:";
    for (std::string const& reqName: missing)
      e << " '" << reqName << "'";
    throw e << ".\n";
  }
  
} // sbndaq::TriggerMonitorWithDecoder::TriggerMonitorWithDecoder()


// -----------------------------------------------------------------------------
void sbndaq::TriggerMonitorWithDecoder::beginRun(art::Run const& run) {
  if (run.run() == fLastRun) return;
  
  resetCounters();
  
  fLastRun = run.run();
  
} // sbndaq::TriggerMonitorWithDecoder::beginRun()


// -----------------------------------------------------------------------------
void sbndaq::TriggerMonitorWithDecoder::resetCounters() {
  
  // these values may be turned into non-hardcoded when the
  // `TriggerConfiguration` data product is moved from `icaruscode` to `sbnobj`
  // or equivalent
  
  // initialization of the counters
  GateCounters_t* gateCounters = nullptr;
  
  sbn::triggerSource trigSource;
  
  trigSource = sbn::triggerSource::BNB;
  gateCounters = &(fGateCounters[value(trigSource)]);
  *gateCounters = GateCounters_t{};
  gateCounters->name               = name(trigSource);
  gateCounters->SpillTimeMaxAlarm  = 1600.0; // ns
  gateCounters->EnableTimeMaxAlarm = 2000000.0; // ns
  
  trigSource = sbn::triggerSource::OffbeamBNB;
  gateCounters = &(fGateCounters[value(trigSource)]);
  *gateCounters = GateCounters_t{};
  gateCounters->name               = name(trigSource);
  gateCounters->SpillTimeMaxAlarm  = 1600.0; // ns
  gateCounters->EnableTimeMaxAlarm = 2000000.0; // ns
  
  trigSource = sbn::triggerSource::NuMI;
  gateCounters = &(fGateCounters[value(trigSource)]);
  *gateCounters = GateCounters_t{};
  gateCounters->name               = name(trigSource);
  gateCounters->SpillTimeMaxAlarm  = 9600.0; // ns
  gateCounters->EnableTimeMaxAlarm = 2000000.0; // ns
  
  trigSource = sbn::triggerSource::OffbeamNuMI;
  gateCounters = &(fGateCounters[value(trigSource)]);
  *gateCounters = GateCounters_t{};
  gateCounters->name               = name(trigSource);
  gateCounters->SpillTimeMaxAlarm  = 9600.0; // ns
  gateCounters->EnableTimeMaxAlarm = 2000000.0; // ns
  
  trigSource = sbn::triggerSource::Calib;
  gateCounters = &(fGateCounters[value(trigSource)]);
  *gateCounters = GateCounters_t{};
  gateCounters->name               = name(trigSource);
  gateCounters->SpillTimeMaxAlarm  = 1600.0; // ns
  gateCounters->EnableTimeMaxAlarm = 2000000.0; // ns
  
} // sbndaq::TriggerMonitorWithDecoder::resetCounters()


// -----------------------------------------------------------------------------
void sbndaq::TriggerMonitorWithDecoder::analyze(art::Event const& event) {
  
  auto trigInfoHandle = event.getHandle<sbn::ExtraTriggerInfo>(fTriggerTag);
  if (!trigInfoHandle) {
    // DQM needs to be robust, so we are unusually forgiving
    mf::LogWarning{ fLogCategory }
      << "No trigger information '" << fTriggerTag.encode()
      << "' present: skipping trigger data quality";
    return;
  }
  
  sbn::ExtraTriggerInfo const& trigInfo = *trigInfoHandle;
  
  mf::LogTrace{ fLogCategory } << "Trigger: " << trigInfo;
  
  monitorTriggerGate(trigInfo);
  
  monitorLVDSbits(trigInfo);
  
} // sbndaq::TriggerMonitorWithDecoder::analyze()


// -----------------------------------------------------------------------------
void sbndaq::TriggerMonitorWithDecoder::monitorTriggerGate
  (sbn::ExtraTriggerInfo const& trigInfo)
{
  //
  // extract information
  //
  double const trigFromBeam = trigInfo.triggerFromBeamGate() / 1000.0; // us
  
  double const trigFromEnable = trigFromBeam
    + (trigInfo.beamGateTimestamp - trigInfo.enableGateTimestamp) / 1000.0;

  // identify which gate we are dealing with:
  std::size_t const gateIndex = value(trigInfo.sourceType);
  GateCounters_t& gateCounters = fGateCounters.at(gateIndex);
  
  //
  // checks
  //
  unsigned int nErrors = 0;
  if (trigFromBeam > gateCounters.SpillTimeMaxAlarm) {
    mf::LogWarning{ fLogCategory }
      << "Global trigger AFTER end of " << gateCounters.name << " beam gate!";
    ++gateCounters.nSpillTimeMaxAlarms;
    ++nErrors;
  }
  
  // enable gate
  if (trigFromEnable > gateCounters.EnableTimeMaxAlarm) {
    mf::LogWarning{ fLogCategory }
      << "Global trigger AFTER end of " << gateCounters.name << " enable gate!";
    ++gateCounters.nEnableTimeMaxAlarms;
    ++nErrors;
  }
  
  if (nErrors > 0) {
    mf::LogInfo{ fLogCategory } << "Trigger information: " << trigInfo;
  }
  
  //
  // send the metrics
  //
  std::string metricsName;
  
  metricsName = gateCounters.name + "spillTime";
  mf::LogTrace{ fLogCategory } << "Sending trigger metric " << metricsName
    << " => " << trigFromBeam << " us";
  fHists.require(metricsName).Fill(trigFromBeam);

  metricsName = gateCounters.name + "enableTime";
  mf::LogTrace{ fLogCategory } << "Sending trigger metric " << metricsName
    << " => " << trigFromEnable << " us";
  fHists.require(metricsName).Fill(trigFromEnable);
  
} // sbndaq::TriggerMonitorWithDecoder::monitorTriggerGate()


// -----------------------------------------------------------------------------
void sbndaq::TriggerMonitorWithDecoder::monitorLVDSbits(
  sbn::ExtraTriggerInfo const& trigInfo
) {
  
  if (trigInfo.triggerLocation() & mask(sbn::triggerLocation::CryoEast)) {
    
    sbn::ExtraTriggerInfo::CryostatInfo const& cryoInfo
      = trigInfo.cryostats[sbn::ExtraTriggerInfo::EastCryostat];
    
    sendConnectorBitMetrics
      ("LVDSbitsEE", cryoInfo.LVDSstatus[sbn::ExtraTriggerInfo::EastPMTwall]);
    sendConnectorBitMetrics
      ("LVDSbitsEW", cryoInfo.LVDSstatus[sbn::ExtraTriggerInfo::WestPMTwall]);
    
  }
  
  if (trigInfo.triggerLocation() & mask(sbn::triggerLocation::CryoWest)) {
    
    sbn::ExtraTriggerInfo::CryostatInfo const& cryoInfo
      = trigInfo.cryostats[sbn::ExtraTriggerInfo::WestCryostat];
    
    sendConnectorBitMetrics
      ("LVDSbitsWE", cryoInfo.LVDSstatus[sbn::ExtraTriggerInfo::EastPMTwall]);
    sendConnectorBitMetrics
      ("LVDSbitsWW", cryoInfo.LVDSstatus[sbn::ExtraTriggerInfo::WestPMTwall]);
    
  }
  
} // sbndaq::TriggerMonitorWithDecoder::monitorLVDSbits()


// -----------------------------------------------------------------------------
void sbndaq::TriggerMonitorWithDecoder::sendConnectorBitMetrics(
  std::string const& metricsName, std::uint64_t connectorBits,
  std::ptrdiff_t offset /* = 0 */
) {
  
  std::bitset<64U> LVDSbits{ connectorBits };
  
  mf::LogTrace log{ fLogCategory };
  log << "LVDS[" << metricsName << "]=0x" << std::hex << connectorBits;
  if (!connectorBits) return;
  
  log << std::dec << ", bits:";
  for (std::size_t const bit: util::counter(64)) {
    if (!LVDSbits[bit]) continue; // bit not set, nothing to do
    
    log << " " << bit;
    
    fHists.require(metricsName).Fill(bit + offset);
    
  } // for
  
} // sbndaq::TriggerMonitorWithDecoder::sendConnectorBitMetrics()


// -----------------------------------------------------------------------------
DEFINE_ART_MODULE(sbndaq::TriggerMonitorWithDecoder)


// -----------------------------------------------------------------------------
