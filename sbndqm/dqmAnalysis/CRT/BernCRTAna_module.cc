////////////////////////////////////////////////////////////////////////
// Class:       BernCRTAna
// Module Type: analyzer
// File:        BernCRTAna_module.cc
// Description: Makes a tree with waveform information.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "sbndaq-artdaq-core/Overlays/Common/BernCRTFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
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
  class BernCRTAna;
}

/*****/

class sbndaq::BernCRTAna : public art::EDAnalyzer {

public:
  explicit BernCRTAna(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~BernCRTAna();
  
  virtual void analyze(art::Event const & evt);
  
private:
  
  //sample histogram
  TH1F* fSampleHist;
  
};

//Define the constructor
sbndaq::BernCRTAna::BernCRTAna(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  //this is how you setup/use the TFileService
  //I do this here in the constructor to setup the histogram just once
  //but you can call the TFileService and make new objects from anywhere
 // art::ServiceHandle<art::TFileService> tfs; //pointer to a file named tfs

  //make the histogram
  //the arguments are the same as what would get passed into ROOT
//  fSampleHist = tfs->make<TH1F>("hSampleHist","Sample Hist Title; x axis (units); y axis (units)",100,-50,50);
///this line
  sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics")); //This causes the error for no "metrics" at the beginning or the end
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));  //This line cauess the code to not be able to compile -- undefined reference to this thing Commenting out this line allows it to compile
}

sbndaq::BernCRTAna::~BernCRTAna()
{
}

void sbndaq::BernCRTAna::analyze(art::Event const & evt)
{
  sleep(2);


  //can get the art event number
  art::EventNumber_t eventNumber = evt.event();

  //we get a 'handle' to the fragments in the event
  //this will act like a pointer to a vector of artdaq fragments
  art::Handle< std::vector<artdaq::Fragment> > rawFragHandle;

  //we fill the handle by getting the right data according to the label
  //the module label will be 'daq', while the instance label (second argument) matches the type of fragment
  evt.getByLabel("daq","BERNCRT", rawFragHandle);

  //this checks to make sure it's ok
  if (!rawFragHandle.isValid()) return;

  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;  
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
            << ", event " << eventNumber << " has " << rawFragHandle->size()
            << " CRT fragment(s)." << std::endl;

  for (size_t idx = 0; idx < rawFragHandle->size(); ++idx) { // loop over the fragments of an event

    const auto& frag((*rawFragHandle)[idx]); // use this fragment as a reference to the same data

    //this applies the 'overlay' to the fragment, to tell it to treat it like a BernCRT fragment
    BernCRTFragment bern_fragment(frag);

    unsigned readout_number = 0; //board number //AA: why is it always 0? what is the purpose?!
    std::string readout_number_str = std::to_string(readout_number);

    //gets a pointers to the data and metadata from the fragment
    BernCRTEvent const* evt = bern_fragment.eventdata();
    const BernCRTFragmentMetadata* md = bern_fragment.metadata();

    std::cout << *evt << std::endl;
    std::cout << *md << std::endl;

    //Get information from the fragment
    //unused variables must be commented, as the compiler treats warnings as errors
    //header:
//    uint64_t fragment_timestamp = frag.timestamp();
//    uint64_t fragment_id = frag.fragmentID();

    //data from FEB:
    int mac5     = evt->MAC5();
//    int flags    = evt->flags;
//    int lostcpu  = evt->lostcpu;
//    int lostfpga = evt->lostfpga;
    int ts0      = evt->Time_TS0();
    int ts1      = evt->Time_TS1();
//    int coinc    = evt->coinc;
    bool     isTS0    = evt->IsReference_TS0();
    bool     isTS1    = evt->IsReference_TS1();

    uint16_t adc[32];
    for(int ch=0; ch<32; ch++) adc[ch] = evt->ADC(ch);

    //metadata
//    uint64_t run_start_time            = md->run_start_time();
//    uint64_t this_poll_start           = md->this_poll_start();
//    uint64_t this_poll_end             = md->this_poll_end();
//    uint64_t last_poll_start           = md->last_poll_start();
//    uint64_t last_poll_end             = md->last_poll_end();
//    int32_t  system_clock_deviation    = md->system_clock_deviation();
//    uint32_t feb_events_per_poll       = md->feb_events_per_poll();
//    uint32_t feb_event_number          = md->feb_event_number();

    //AA: I propose to keep all the code getting fragment data above


    size_t max        = 0;
    size_t totaladc   = 0;
    size_t ADCchannel = 0;


    //    std::string FEBID_str = std::to_string(fragment_id);
    //    sbndaq::sendMetric("CRT_board", FEBID_str, "FEBID", fragment_id, 0, artdaq::MetricMode::LastPoint); 


    //let's fill our sample hist with the Time_TS0()-1e9 if 
    //it's a GPS reference pulse
    if(isTS0){
      fSampleHist->Fill(ts0 - 1e9);
      std::cout<<" TS0 "<<ts0 - 1e9<<std::endl;
    }
    if(isTS1){
      fSampleHist->Fill(ts1 - 1e9);
      std::cout<<" TS1 "<<ts0 - 1e9<<std::endl; //AA: why do we display ts0 here?
    }
    for(int i = 0; i<32; i++) {
      std::cout<<i<<": "<<adc[i]<<std::endl;
      totaladc  += adc[i];
      ADCchannel = adc[i];
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average);
      if(adc[i] > max){
        max = adc[i];
      }
    }
    int baseline = (totaladc-max)/31;
    std::cout<<" Maximum ADC value:"<<max<<std::endl;
    std::cout<<" Average without Max value:"<<baseline<<std::endl;
    sbndaq::sendMetric("CRT_board", readout_number_str, "MaxADCValue", max, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric("CRT_board", readout_number_str, "baseline", baseline, 0, artdaq::MetricMode::Average);

    //Per board front end metric group
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS0", ts0, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS1", ts1, 0, artdaq::MetricMode::LastPoint);


    //ADC Analysis metric group



  }//end loop over fragments


}

DEFINE_ART_MODULE(sbndaq::BernCRTAna)
//this is where the name is specified
