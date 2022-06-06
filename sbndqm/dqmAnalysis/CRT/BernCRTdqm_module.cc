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

#include "sbndaq-artdaq-core/Overlays/Common/BernCRTTranslator.hh"

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
  bool IsSideCRT(const icarus::crt::BernCRTTranslator & hit);

   uint64_t lastbighit[32];

  uint16_t silly = 0;

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

bool sbndaq::BernCRTdqm::IsSideCRT(const icarus::crt::BernCRTTranslator & hit) {
  /**
   * Fragment ID described in SBN doc 16111
   */
  return (hit.fragment_ID & 0x3100) == 0x3100;
}


void sbndaq::BernCRTdqm::analyze(art::Event const & evt) {
  //sleep(2);

  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;  
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();

  std::vector<icarus::crt::BernCRTTranslator> hit_vector;

  auto fragmentHandles = evt.getMany<artdaq::Fragments>();
  for (auto  handle : fragmentHandles) {
    if (!handle.isValid() || handle->size() == 0)
      continue;

    auto this_hit_vector = icarus::crt::BernCRTTranslator::getCRTData(*handle);

    hit_vector.insert(hit_vector.end(),this_hit_vector.begin(),this_hit_vector.end());
  }

  //loop over all CRT hits in an event
  for(const auto & hit : hit_vector) {

    enum Detector {SIDE_CRT, TOP_CRT};
   const Detector detector = IsSideCRT(hit) ? SIDE_CRT : TOP_CRT;
    const uint16_t & fragment_id        = hit.fragment_ID;
    /**
     * TODO:
     * In order to distinguish between Top and Side CRT
     * use the variable detector, defined above
     * Otherwise, MAC address alone is not sufficient,
     * as some MACs overlap between Top and Side
     *
     * Alternative: use fragment_ID directly (fragment_IDs
     * are unique)
     */
    const uint64_t & fragment_timestamp = hit.timestamp;

    //data from FEB:
    const uint8_t & mac5     = hit.mac5;
    unsigned readout_number  = mac5;
    std::string readout_number_str = std::to_string(readout_number);

    const int ts0      = hit.ts0;
    const int ts1      = hit.ts1;
    const bool     isTS0    = hit.IsReference_TS0();
    const bool     isTS1    = hit.IsReference_TS1();

    const uint16_t * adc = hit.adc;

    silly++;
    for(int ch=0; ch<32; ch++) {
      if( adc[ch] > 600 ) {
        //cout << ts1 << std::endl;
        //sbndaq::BernCRTdqm::lastbighit[ch] = ts1 - 1e9;
        //std::cout << "Okay a big hit!";
        //std::cout << frag.timestamp() << "\n";
        sbndaq::BernCRTdqm::lastbighit[ch] = fragment_timestamp;
        //std::cout << sbndaq::BernCRTdqm::lastbighit[ch] << "\n";

      }
    }
    const uint64_t & this_poll_end             = hit.this_poll_end;
    const uint64_t & last_poll_start           = hit.last_poll_start;

    size_t max        = 0;
    size_t totaladc   = 0;
    size_t ADCchannel = 0;


    std::string FEBID_str = std::to_string(fragment_id);
    sbndaq::sendMetric("CRT_board", FEBID_str, "FEBID", fragment_id, 0, artdaq::MetricMode::LastPoint); 
    sbndaq::sendMetric("CRT_board_top", FEBID_str, "FEBID", fragment_id, 0, artdaq::MetricMode::LastPoint);

    //let's fill our sample hist with the Time_TS0()-1e9 if 
    //it's a GPS reference pulse
    if(isTS0){
      //    fSampleHist->Fill(ts0 - 1e9); //bug!!! fSampleHist is not defined
      //std::cout<<" TS0 "<<ts0 - 1e9<<std::endl;
    }
    if(isTS1){
      //    fSampleHist->Fill(ts1 - 1e9); //bug!!! fSampleHist is not defined
      //std::cout<<" TS1 "<<ts0 - 1e9<<std::endl; 
    }
    //std::cout<<" Maximum ADC value:"<<max<<std::endl;
    //std::cout<<" Average without Max value:"<<baseline<<std::endl;
    //std::cout<<" CRT_board number:" << readout_number_str<<std::endl;
    //  std::cout<<" TS1: " << ts1 << std::endl;
    //  std::cout<<" TSO: " << ts0 << std::endl;
    //  std::cout<<" last_poll_start   : " << last_poll_start << std::endl;
    //  std::cout<<" fragment_timestamp: " << fragment_timestamp << std::endl;
    //  std::cout<<" this_poll_end     : " << this_poll_end << std::endl;
    //  for(int i = 0; i < 32; i++ ){
    //    std::cout<<" adc " << i << ": " << adc[i];
    //    std::cout<<" lastbighit " << i << ": " << sbndaq::BernCRTdqm::lastbighit[i] << std::endl;
    //
    //  }
    //  std::cout<<" silly: " << silly << std::endl;

    uint64_t earlysynch = last_poll_start - fragment_timestamp;
    uint64_t latesynch = fragment_timestamp - this_poll_end;

    if(detector==SIDE_CRT){
   
    for(int i = 0; i<32; i++) {
      totaladc  += adc[i];
      ADCchannel = adc[i];
      uint64_t lastbighitchannel = fragment_timestamp -sbndaq::BernCRTdqm::lastbighit[i];
      /////    RMSchannel = rms[i];
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average);
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "lastbighit", lastbighitchannel, 0, artdaq::MetricMode::Average);

      if(adc[i] > max){
        max = adc[i];
      }
    }
    int baseline = (totaladc-max)/31;
 
    sbndaq::sendMetric("CRT_board", readout_number_str, "MaxADCValue", max, 0, artdaq::MetricMode::Average);

    sbndaq::sendMetric("CRT_board", readout_number_str, "baseline", baseline, 0, artdaq::MetricMode::Average);

    //Per board front end metric group
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS0", ts0, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS1", ts1, 0, artdaq::MetricMode::LastPoint);

    //Sychronization Metrics
    sbndaq::sendMetric("CRT_board", readout_number_str, "earlysynch", earlysynch, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric("CRT_board", readout_number_str, "latesynch", latesynch, 0, artdaq::MetricMode::Average);
   
     }//select side CRT

  if(detector==TOP_CRT){

    std::vector<double> adcvalues;
    for(int i = 0; i<32; i++) {
      totaladc  += adc[i];
      ADCchannel = adc[i];
      adcvalues.push_back(adc[i]);
      uint64_t lastbighitchannel = fragment_timestamp -sbndaq::BernCRTdqm::lastbighit[i];
      /////    RMSchannel = rms[i];
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average); 
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "lastbighit", lastbighitchannel, 0, artdaq::MetricMode::Average); 
    }
    sort(adcvalues.begin(), adcvalues.end(), std::greater<int>());
    adcvalues.erase(adcvalues.begin(), adcvalues.begin()+3);
    int sum = 0;
    for (auto& n : adcvalues)
     sum += n;
    int baseline = sum/adcvalues.size();
    sbndaq::sendMetric("CRT_board_top", readout_number_str, "MaxADCValue", max, 0, artdaq::MetricMode::Average);

    sbndaq::sendMetric("CRT_board_top", readout_number_str, "baseline", baseline, 0, artdaq::MetricMode::Average);

    //Per board front end metric group
    sbndaq::sendMetric("CRT_board_top", readout_number_str, "TS0", ts0, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board_top", readout_number_str, "TS1", ts1, 0, artdaq::MetricMode::LastPoint);

    //Sychronization Metrics
    sbndaq::sendMetric("CRT_board_top", readout_number_str, "earlysynch", earlysynch, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric("CRT_board_top", readout_number_str, "latesynch", latesynch, 0, artdaq::MetricMode::Average);

   }//select top CRT
  } //loop over all CRT hits in an event

} //analyze


DEFINE_ART_MODULE(sbndaq::BernCRTdqm)
//this is where the name is specified
