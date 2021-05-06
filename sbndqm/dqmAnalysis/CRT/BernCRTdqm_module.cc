////////////////////////////////////////////////////////////////////////
// Class:       BernCRTdqm
// Module Type: analyzer
// File:        BernCRTdqm_module.cc
// Description: Makes a tree with waveform information.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "sbndaq-artdaq-core/Overlays/Common/BernCRTFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
//add these
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
//---
//#include "art/Framework/Services/Optional/TFileService.h"

#include "icaruscode/CRT/CRTDecoder/BernCRTTranslator.hh"

#include "TH1F.h"
#include "TNtuple.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <unistd.h>


namespace sbndaq {
  class BernCRTdqm;
}

/*****/

class sbndaq::BernCRTdqm : public art::EDAnalyzer {

public:
  explicit BernCRTdqm(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~BernCRTdqm();
  
  virtual void analyze(art::Event const & evt);
  
private:
  //sample histogram
  TH1F* fSampleHist;
  
};

//Define the constructor
sbndaq::BernCRTdqm::BernCRTdqm(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  //if (p.has_key("metric_config")) {
  //  sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_config"));
  //}
  //sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics")); //This causes the error for no "metrics" at the beginning or the end
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));  //This line cauess the code to not be able to compile -- undefined reference to this thing Commenting out this line allows it to compile
}

sbndaq::BernCRTdqm::~BernCRTdqm()
{
}


void sbndaq::BernCRTdqm::analyze(art::Event const & evt) {
  sleep(2);

  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;  
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();

  const std::vector<icarus::crt::BernCRTTranslator> hit_vector =  icarus::crt::BernCRTTranslator::getCRTData(evt);

  for(auto & hit : hit_vector) {
    TLOG(TLVL_INFO)<<hit;

    //data from FEB:
    int mac5     = hit.mac5;
    unsigned readout_number = mac5;
    std::string readout_number_str = std::to_string(readout_number);

    int ts0      = hit.ts0; 
    int ts1      = hit.ts1; 
    bool     isTS0    = hit.IsReference_TS0();
    bool     isTS1    = hit.IsReference_TS1();

    uint16_t adc[32];
    //  uint16_t rms[32];
    /////  uint16_t& waveform[32];

    for(int ch=0; ch<32; ch++) {
      adc[ch] = hit.adc[ch];
      /////     waveform = evt->ADC(ch);
      /////     short baseline = Median(waveform, waveform.size());
      /////     double rms = RMS( waveform, waveform.size(), baseline);
    }

    size_t max        = 0;
    size_t totaladc   = 0;
    size_t ADCchannel = 0;


    //    std::string FEBID_str = std::to_string(fragment_id);
    //    sbndaq::sendMetric("CRT_board", FEBID_str, "FEBID", fragment_id, 0, artdaq::MetricMode::LastPoint); 


    //let's fill our sample hist with the Time_TS0()-1e9 if 
    //it's a GPS reference pulse
    if(isTS0){
      //    fSampleHist->Fill(ts0 - 1e9); //bug!!! fSampleHist is not defined
      std::cout<<" TS0 "<<ts0 - 1e9<<std::endl;
    }
    if(isTS1){
      //    fSampleHist->Fill(ts1 - 1e9); //bug!!! fSampleHist is not defined
      std::cout<<" TS1 "<<ts0 - 1e9<<std::endl; 
    }
    for(int i = 0; i<32; i++) {
      totaladc  += adc[i];
      ADCchannel = adc[i];
      /////    RMSchannel = rms[i];
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average); 

      if(adc[i] > max){
        max = adc[i];
      }
    }
    int baseline = (totaladc-max)/31;
    std::cout<<" Maximum ADC value:"<<max<<std::endl;
    std::cout<<" Average without Max value:"<<baseline<<std::endl;
    std::cout<<" CRT_board number:" << readout_number_str<<std::endl;
    sbndaq::sendMetric("CRT_board", readout_number_str, "MaxADCValue", max, 0, artdaq::MetricMode::Average);

    sbndaq::sendMetric("CRT_board", readout_number_str, "baseline", baseline, 0, artdaq::MetricMode::Average);

    //Per board front end metric group
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS0", ts0, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS1", ts1, 0, artdaq::MetricMode::LastPoint);

    /////////////Other metrics: rms like in PMT stuff


    //ADC Analysis metric group

  } //loop over hits in event
} //analyze


DEFINE_ART_MODULE(sbndaq::BernCRTdqm)
//this is where the name is specified
