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
  void analyze_fragment(artdaq::Fragment & frag);  
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


void sbndaq::BernCRTdqm::analyze_fragment(artdaq::Fragment & frag) {

  BernCRTFragment bern_fragment(frag);

  //gets a pointers to the data and metadata from the fragment
  BernCRTEvent const* evt = bern_fragment.eventdata();
  const BernCRTFragmentMetadata* md = bern_fragment.metadata();

//  std::cout << *evt << std::endl;
//  std::cout << *md << std::endl;

  //Get information from the fragment
  //unused variables must be commented, as the compiler treats warnings as errors
  //header:
      uint64_t fragment_timestamp = frag.timestamp();
      uint64_t fragment_id        = frag.fragmentID();

  //data from FEB:
  int mac5     = evt->MAC5();
  unsigned readout_number = mac5;
  std::string readout_number_str = std::to_string(readout_number);

  //    int flags    = evt->flags;
  //    int lostcpu  = evt->lostcpu;
  //    int lostfpga = evt->lostfpga;
  //    int flags    = evt->flags;
  //    int lostcpu  = evt->lostcpu;
  //    int lostfpga = evt->lostfpga;
  int ts0      = evt->Time_TS0();
  int ts1      = evt->Time_TS1();
  //    int coinc    = evt->coinc;
  bool     isTS0    = evt->IsReference_TS0();
  bool     isTS1    = evt->IsReference_TS1();

  uint16_t adc[32];

  silly++;
  for(int ch=0; ch<32; ch++) {
     adc[ch] = evt->ADC(ch);
     if( adc[ch] > 600 ) {
       //cout << ts1 << std::endl;
       //sbndaq::BernCRTdqm::lastbighit[ch] = ts1 - 1e9;
       //std::cout << "Okay a big hit!";
       //std::cout << frag.timestamp() << "\n";
       sbndaq::BernCRTdqm::lastbighit[ch] = frag.timestamp();
       //std::cout << sbndaq::BernCRTdqm::lastbighit[ch] << "\n";

     }
   }
  //metadata
  //    uint64_t run_start_time            = md->run_start_time();
//      uint64_t this_poll_start           = md->this_poll_start();
      uint64_t this_poll_end             = md->this_poll_end();
      uint64_t last_poll_start           = md->last_poll_start();
//      uint64_t last_poll_end             = md->last_poll_end();
  //    int32_t  system_clock_deviation    = md->system_clock_deviation();
  //    uint32_t feb_events_per_poll       = md->feb_events_per_poll();
  //    uint32_t feb_event_number          = md->feb_event_number();

  //AA: I propose to keep all the code getting fragment data above


  size_t max        = 0;
  size_t totaladc   = 0;
  size_t ADCchannel = 0;


      std::string FEBID_str = std::to_string(fragment_id);
      sbndaq::sendMetric("CRT_board", FEBID_str, "FEBID", fragment_id, 0, artdaq::MetricMode::LastPoint); 


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
    uint64_t lastbighitchannel = fragment_timestamp -sbndaq::BernCRTdqm::lastbighit[i];
/////    RMSchannel = rms[i];
    sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average); 
    sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "lastbighit", lastbighitchannel, 0, artdaq::MetricMode::Average); 

    if(adc[i] > max){
      max = adc[i];
    }
  }
  int baseline = (totaladc-max)/31;
//  std::cout<<" Maximum ADC value:"<<max<<std::endl;
//  std::cout<<" Average without Max value:"<<baseline<<std::endl;
//  std::cout<<" CRT_board number:" << readout_number_str<<std::endl;
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

  sbndaq::sendMetric("CRT_board", readout_number_str, "MaxADCValue", max, 0, artdaq::MetricMode::Average);

  sbndaq::sendMetric("CRT_board", readout_number_str, "baseline", baseline, 0, artdaq::MetricMode::Average);

  //Per board front end metric group
  sbndaq::sendMetric("CRT_board", readout_number_str, "TS0", ts0, 0, artdaq::MetricMode::LastPoint);
  sbndaq::sendMetric("CRT_board", readout_number_str, "TS1", ts1, 0, artdaq::MetricMode::LastPoint);

  //Sychronization Metrics
  sbndaq::sendMetric("CRT_board", readout_number_str, "earlysynch", earlysynch, 0, artdaq::MetricMode::Average);
  sbndaq::sendMetric("CRT_board", readout_number_str, "latesynch", latesynch, 0, artdaq::MetricMode::Average);



} //analyze_fragment

void sbndaq::BernCRTdqm::analyze(art::Event const & evt) {
  sleep(2);

  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;  
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();

  //loop over fragments in event
  //two different loop logics, depending on whether we have fragment containers or fragments
  auto fragmentHandles = evt.getMany<artdaq::Fragments>();
  for (auto handle : fragmentHandles) {
    if (!handle.isValid() || handle->size() == 0)
      continue;



    if (handle->front().type() == artdaq::Fragment::ContainerFragmentType) {
      //Container fragment
      for(int i = 0; i < 32; i++ ){
        lastbighit[i]=-1;
      }
      for (auto cont : *handle) {
        artdaq::ContainerFragment contf(cont);
        if (contf.fragment_type() != sbndaq::detail::FragmentType::BERNCRT)
          continue;
        std::cout << " has " <<  contf.block_count() << " CRT fragment(s)." << std::endl;
        for (size_t ii = 0; ii < contf.block_count(); ++ii)
          analyze_fragment(*contf[ii].get());
      }
    }
    else {
      //normal fragment
      if (handle->front().type() != sbndaq::detail::FragmentType::BERNCRT) continue;
      std::cout << " has " << handle->size() << " CRT fragment(s)." << std::endl; 
      for (auto frag : *handle) 
        analyze_fragment(frag);
    }
  }
} //analyze


DEFINE_ART_MODULE(sbndaq::BernCRTdqm)
//this is where the name is specified
