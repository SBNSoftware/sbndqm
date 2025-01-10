////////////////////////////////////////////////////////////////////////
// Class:       CAENV1730FlashMetricsSBND
// Plugin Type: analyzer (Unknown Unknown)
// File:        CAENV1730FlashMetricsSBND_module.cc
//
// Generated at Wed May 29 12:46:34 2024 by Lynn Tung using cetskelgen
// from  version .
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "sbndaq-artdaq-core/Obj/SBND/pmtSoftwareTrigger.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/PTBFragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"

#include "sbndqm/dqmAnalysis/Utils/SBNDHLTFilterUtils.hh"

#include "sbndqm/Decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"


class CAENV1730FlashMetricsSBND;


class CAENV1730FlashMetricsSBND : public art::EDAnalyzer {
public:
  explicit CAENV1730FlashMetricsSBND(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  CAENV1730FlashMetricsSBND(CAENV1730FlashMetricsSBND const&) = delete;
  CAENV1730FlashMetricsSBND(CAENV1730FlashMetricsSBND&&) = delete;
  CAENV1730FlashMetricsSBND& operator=(CAENV1730FlashMetricsSBND const&) = delete;
  CAENV1730FlashMetricsSBND& operator=(CAENV1730FlashMetricsSBND&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

private:

  // Declare member data here.
  art::InputTag m_flashmetric_tag;

  std::string   m_ptb_instance;
  std::vector<uint64_t> m_HLT_beamzero;
  std::vector<uint64_t> m_HLT_beamlight;
  std::vector<uint64_t> m_HLT_beam_excluded;
  std::vector<uint64_t> m_HLT_offbeamzero;
  std::vector<uint64_t> m_HLT_offbeamlight;
  std::vector<uint64_t> m_HLT_offbeam_excluded;
  std::vector<uint64_t> m_HLT_crossingmuon;
  std::vector<uint64_t> m_HLT_crossingmuon_excluded;

  float         m_flashpeak_thresh;
  int          getFileStream(art::Handle<std::vector<artdaq::Fragment> > ptb_handle);
};


CAENV1730FlashMetricsSBND::CAENV1730FlashMetricsSBND(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  , m_flashmetric_tag{ p.get<art::InputTag>("FlashMetricLabel") }
  , m_ptb_instance{ p.get<std::string>("PTBInstanceLabel","ContainerPTB") }
  , m_HLT_beamzero{ p.get<std::vector<uint64_t>>("HLT_beamzero") }
  , m_HLT_beamlight{ p.get<std::vector<uint64_t>>("HLT_beamlight") }
  , m_HLT_beam_excluded{ p.get<std::vector<uint64_t>>("HLT_beam_excluded") }
  , m_HLT_offbeamzero{ p.get<std::vector<uint64_t>>("HLT_offbeamzero") }
  , m_HLT_offbeamlight{ p.get<std::vector<uint64_t>>("HLT_offbeamlight") }
  , m_HLT_offbeam_excluded{ p.get<std::vector<uint64_t>>("HLT_offbeam_excluded") }
  , m_HLT_crossingmuon{ p.get<std::vector<uint64_t>>("HLT_crossingmuon") }
  , m_HLT_crossingmuon_excluded{ p.get<std::vector<uint64_t>>("HLT_crossingmuon_excluded") }
  , m_flashpeak_thresh{ p.get<float>("FlashPeakThreshold",100) }
{
  if (p.has_key("metrics")) {
    sbndaq::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
  }
  sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_flashes"));
}

void CAENV1730FlashMetricsSBND::analyze(art::Event const& e)
{
  int level = 3;

  art::Handle<std::vector<artdaq::Fragment> > ptb_handle;
  e.getByLabel("daq", m_ptb_instance, ptb_handle);
  auto stream = getFileStream(ptb_handle);
  std::cout << "Stream: " << stream << std::endl;

  art::Handle pmtmetricHandle = e.getHandle<std::vector<sbnd::trigger::pmtSoftwareTrigger>>(m_flashmetric_tag);
  if( pmtmetricHandle.isValid() && !pmtmetricHandle->empty() ) {
    for (auto const & pmtmetric : *pmtmetricHandle) {
      auto ts = pmtmetric.peaktime;
      auto pe = pmtmetric.peakPE;
      
      std::cout << "Flash ts: " << ts << " PE: " << pe << std::endl;
      if (pe>100){
        sbndaq::sendMetric("BeamMetrics","0","flash_ts", ts, level, artdaq::MetricMode::LastPoint);
        sbndaq::sendMetric("BeamMetrics","0","flash_pe", pe, level, artdaq::MetricMode::LastPoint);
      }
    }
  }
}

int CAENV1730FlashMetricsSBND::getFileStream(art::Handle<std::vector<artdaq::Fragment> > ptb_handle){
  int stream = -1;
  std::vector<uint64_t> hlt_vec;

  for (auto const& cont : *ptb_handle){
    std::cout << "Processing container" << std::endl;
    artdaq::ContainerFragment contf(cont);
    auto these_hlts = sbndqm::SBNDHLTFilterUtils::GetAllHLTs(&contf);
    hlt_vec.insert(hlt_vec.end(), these_hlts.begin(), these_hlts.end());
  }

  std::cout << "HLT size = " << hlt_vec.size() << ", contains HLT = ";
  for (auto const hlt: hlt_vec){
    std::cout << hlt << " ";
  }
  bool passBeamZero = false;
  bool passBeamLight = false;
  bool passOffbeamZero = false;
  bool passOffbeamLight = false;
  bool passXmuon = false;

  passBeamZero = sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, m_HLT_beamzero, m_HLT_beam_excluded);
  passBeamLight= sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, m_HLT_beamlight, m_HLT_beam_excluded);
  passOffbeamZero = sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, m_HLT_offbeamzero, m_HLT_offbeam_excluded);
  passOffbeamLight = sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, m_HLT_offbeamlight, m_HLT_offbeam_excluded);
  passXmuon   = sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, m_HLT_crossingmuon, m_HLT_crossingmuon_excluded);

  if (passBeamZero) stream = 1;
  if (passBeamLight) stream = 2;
  if (passOffbeamZero) stream = 3;
  if (passOffbeamLight) stream = 4;
  if (passXmuon) stream = 5;

  return stream;
}

DEFINE_ART_MODULE(CAENV1730FlashMetricsSBND)
