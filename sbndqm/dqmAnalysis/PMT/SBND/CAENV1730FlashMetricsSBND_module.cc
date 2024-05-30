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
};


CAENV1730FlashMetricsSBND::CAENV1730FlashMetricsSBND(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  , m_flashmetric_tag{ p.get<art::InputTag>("FlashMetricLabel") }
{
  if (p.has_key("metrics")) {
    sbndaq::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
  }
  sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_flashes"));
}

void CAENV1730FlashMetricsSBND::analyze(art::Event const& e)
{
  int level = 3;

  art::Handle pmtmetricHandle = e.getHandle<std::vector<sbnd::trigger::pmtSoftwareTrigger>>(m_flashmetric_tag);
  if( pmtmetricHandle.isValid() && !pmtmetricHandle->empty() ) {
    for (auto const & pmtmetric : *pmtmetricHandle) {
      auto ts = pmtmetric.peaktime;
      auto pe = pmtmetric.peakPE;
      std::cout << "Flash ts: " << ts << " PE: " << pe << std::endl;
      sbndaq::sendMetric("flash_ts", ts, level, artdaq::MetricMode::LastPoint);
      sbndaq::sendMetric("flash_pe", pe, level, artdaq::MetricMode::LastPoint);
    }
  }
}

DEFINE_ART_MODULE(CAENV1730FlashMetricsSBND)
