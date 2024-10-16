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
  const size_t fNReport;

  std::map<int,size_t> fNtrks;
  std::map<int,double> fAttenSum;

  std::string CryoToString(int) const;

};


dqm::TPCPurityDQMSender::TPCPurityDQMSender(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  , fPurityInfoLabels(p.get< std::vector<art::InputTag> >("PurityInfoLabels"))
  , fPrintInfo(p.get<bool>("PrintInfo",true))
  , fNReport(p.get<size_t>("MinTracksToReport",1))
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

std::string dqm::TPCPurityDQMSender::CryoToString(int c) const
{
  if(c==0)
    return std::string("EastCryostat");
  else if(c==1)
    return std::string("WestCryostat");
  
  return std::string("UnknownCryostat");
}
  
void dqm::TPCPurityDQMSender::beginJob(){
}

void dqm::TPCPurityDQMSender::analyze(art::Event const& e)
{

  if(fPrintInfo){
    std::cout << "Processing Run " << e.run() 
	      << ", Subrun " << e.subRun()
	      << ", Event " << e.event() 
	      << ":" << std::endl;
    for(auto const& x : fNtrks){
      std::cout << "\t Cryo " << x.first << " : " 
		<< x.second << " / " << fNReport << " tracks."
		<< std::endl;
    }
  }

  for( auto const& label : fPurityInfoLabels){
    art::Handle< std::vector<anab::TPCPurityInfo> > purityInfoHandle;
    e.getByLabel(label,purityInfoHandle);
    auto const& purityInfoVec(*purityInfoHandle);
  
    if(fPrintInfo)
      std::cout << "\tThere are " << purityInfoVec.size() << " purity info objects in the event."
		<< std::endl;
  
    for(auto const& pinfo : purityInfoVec){
      if(fPrintInfo) pinfo.Print();

      fNtrks[pinfo.Cryostat] = fNtrks[pinfo.Cryostat]+1;
      fAttenSum[pinfo.Cryostat] = fAttenSum[pinfo.Cryostat] + 1000.*pinfo.Attenuation;


      if(fNtrks[pinfo.Cryostat]>=fNReport){
	auto instance = CryoToString(pinfo.Cryostat);

	if(fPrintInfo) std::cout << "Sending out " << instance << " attenuation_raw=" << fAttenSum[pinfo.Cryostat]/fNtrks[pinfo.Cryostat] << std::endl;
	sbndaq::sendMetric("tpc_purity", instance, "attenuation_raw", fAttenSum[pinfo.Cryostat]/fNtrks[pinfo.Cryostat], 0, artdaq::MetricMode::Average);
	if(fPrintInfo) std::cout << "Sending out " << instance << " lifetime_raw=" << 1./(fAttenSum[pinfo.Cryostat]/fNtrks[pinfo.Cryostat]) << std::endl;
	sbndaq::sendMetric("tpc_purity", instance, "lifetime_raw", fAttenSum[pinfo.Cryostat]/fNtrks[pinfo.Cryostat], 0, artdaq::MetricMode::Average,"INVERT/");
      
	fNtrks[pinfo.Cryostat]=0;
	fAttenSum[pinfo.Cryostat]=0;
      }
    }
  }
}

DEFINE_ART_MODULE(dqm::TPCPurityDQMSender)
