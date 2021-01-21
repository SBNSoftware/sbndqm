////////////////////////////////////////////////////////////////////////
// Class:       TPCPurityDQMSender
// Plugin Type: analyzer (art v3_04_00)
// File:        TPCPurityDQMSender_module.cc
//
// Generated at Sun Jan 26 22:13:22 2020 by Wesley Ketchum using cetskelgen
// from cetlib version v3_09_00.
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

//purity info class
#include "sbnobj/Common/Analysis/TPCPurityInfo.hh"

//output to DQM
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

namespace dqm { class TPCPurityDQMSender; }


class dqm::TPCPurityDQMSender : public art::EDAnalyzer {
  public:
  
  explicit TPCPurityDQMSender(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  TPCPurityDQMSender(TPCPurityDQMSender const&) = delete;
  TPCPurityDQMSender(TPCPurityDQMSender&&) = delete;
  TPCPurityDQMSender& operator=(TPCPurityDQMSender const&) = delete;
  TPCPurityDQMSender& operator=(TPCPurityDQMSender&&) = delete;

  void analyze(art::Event const& e) override;
  void beginJob() override;

private:

  // Declare member data here.
  std::vector<art::InputTag> const fPurityInfoLabels;
  bool fPrintInfo;

};


dqm::TPCPurityDQMSender::TPCPurityDQMSender(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  , fPurityInfoLabels(p.get< std::vector<art::InputTag> >("PurityInfoLabels"))
  , fPrintInfo(p.get<bool>("PrintInfo",true))
{
  for(auto const& label : fPurityInfoLabels)
    consumes< std::vector<anab::TPCPurityInfo> >(label);

  if (p.has_key("metrics")) {
    sbndaq::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
  }
  if (p.has_key("metric_config")) {
    sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_config"));
  }
}
  
void dqm::TPCPurityDQMSender::beginJob(){

}

void dqm::TPCPurityDQMSender::analyze(art::Event const& e)
{

  if(fPrintInfo)
    std::cout << "Processing Run " << e.run() 
	      << ", Subrun " << e.subRun()
	      << ", Event " << e.event() 
	      << ":" << std::endl;
  
  for( auto const& label : fPurityInfoLabels){
    art::Handle< std::vector<anab::TPCPurityInfo> > purityInfoHandle;
    e.getByLabel(label,purityInfoHandle);
    auto const& purityInfoVec(*purityInfoHandle);
  
    if(fPrintInfo)
      std::cout << "\tThere are " << purityInfoVec.size() << " purity info objects in the event."
		<< std::endl;
  
    for(auto const& pinfo : purityInfoVec){
      if(fPrintInfo) pinfo.Print();

      std::string instance=std::to_string(pinfo.Cryostat);
      sbndaq::sendMetric("tpc_purity", instance, "attenuation", pinfo.Attenuation, 0, artdaq::MetricMode::Average);
      sbndaq::sendMetric("tpc_purity", instance, "INVERT/lifetime", pinfo.Attenuation, 0, artdaq::MetricMode::Average);
      
    }
  }  
  
}

DEFINE_ART_MODULE(dqm::TPCPurityDQMSender)
