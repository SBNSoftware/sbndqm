////////////////////////////////////////////////////////////////////////
// Class:       BernCRTZMQAna
// Module Type: analyzer
// File:        BernCRTZMQAna_module.cc
// Description: Makes a tree with waveform information.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "sbndaq-artdaq-core/Overlays/Common/BernCRTZMQFragment.hh"
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
  class BernCRTZMQAna;
}

/*****/

class sbndaq::BernCRTZMQAna : public art::EDAnalyzer {

public:
  explicit BernCRTZMQAna(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~BernCRTZMQAna();
  
  virtual void analyze(art::Event const & evt);
  
private:
  
  //sample histogram
  TH1F* fSampleHist;
  
};

//Define the constructor
sbndaq::BernCRTZMQAna::BernCRTZMQAna(fhicl::ParameterSet const & pset)
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

sbndaq::BernCRTZMQAna::~BernCRTZMQAna()
{
}

void sbndaq::BernCRTZMQAna::analyze(art::Event const & evt)
{
sleep(2);


  //can get the art event number
  art::EventNumber_t eventNumber = evt.event();
  
  //we get a 'handle' to the fragments in the event
  //this will act like a pointer to a vector of artdaq fragments
  art::Handle< std::vector<artdaq::Fragment> > rawFragHandle;
  
  //we fill the handle by getting the right data according to the label
  //the module label will be 'daq', while the instance label (second argument) matches the type of fragment
  evt.getByLabel("daq","BERNCRTZMQ", rawFragHandle);

  //this checks to make sure it's ok
  if (!rawFragHandle.isValid()) return;
  
  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;  
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
	    << ", event " << eventNumber << " has " << rawFragHandle->size()
	    << " CRT fragment(s)." << std::endl;
  
  for (size_t idx = 0; idx < rawFragHandle->size(); ++idx) { // loop over the fragments of an event

 
    const auto& frag((*rawFragHandle)[idx]); // use this fragment as a reference to the same data

    //this applies the 'overlay' to the fragment, to tell it to treat it like a BernCRTZMQ fragment
    BernCRTZMQFragment bb(frag);
    
    unsigned readout_number = 0; //board number
    std::string readout_number_str = std::to_string(readout_number);
    //gets a pointer to the metadata from the fragment
    //type is BernCRTZMQFragmentMetadata*
    auto const* md = bb.metadata();
    std::cout << *md << std::endl;

    //gets the number of CRT events (hits) inside this fragment
    size_t n_events = md->n_events();
    std::cout<<"n_event value:"<<n_events<<std::endl;
    sbndaq::sendMetric("CRT_board", readout_number_str, "n_events", n_events, 0, artdaq::MetricMode::Average);
 
    size_t max        = 0;
    size_t totaladc   = 0;
    size_t ADCchannel = 0;

    //loop over the events in the fragment ...
    for(size_t i_e=0; i_e<n_events; ++i_e){
      
      //grab the pointer to the event
      BernCRTZMQEvent const* evt = bb.eventdata(i_e);

      size_t FEBID = evt->MAC5();
      std::string FEBID_str = std::to_string(FEBID);

//    sbndaq::sendMetric("CRT_board", FEBID_str, "FEBID", FEBID, 0, artdaq::MetricMode::LastPoint); 

      unsigned readout_number_channel = 32*(FEBID-1);
	
      std::string readout_number_channel_str = std::to_string(readout_number_channel);

      //we can print this
      std::cout << *evt << std::endl;

      //let's fill our sample hist with the Time_TS0()-1e9 if 
      //it's a GPS reference pulse
      if(evt->IsReference_TS0()){
	fSampleHist->Fill(evt->Time_TS0()-1e9);
        std::cout<<" TS0 "<<evt->Time_TS0()-1e9<<std::endl; //nothing is changed in the output when i added this line, and it compiles just fine
       //cout<<"I printed here"<<endl;
      }
      if(evt->IsReference_TS1()){
	fSampleHist->Fill(evt->Time_TS1()-1e9);
        std::cout<<" TS1 "<<evt->Time_TS0()-1e9<<std::endl;
	//adding this to teh output cause a segmentation fault (core dumped)
      }
      for(int i = 0; i<32; i++){
  	std::cout<<i+1<<": "<<evt->ADC(i)<<std::endl;
	totaladc  += evt->ADC(i);
	ADCchannel = evt->ADC(i);
	sbndaq::sendMetric("CRT_channel", readout_number_channel_str, "ADC", ADCchannel, 0, artdaq::MetricMode::Average);
	readout_number_channel += 1;
	readout_number_channel_str = std::to_string(readout_number_channel);
	if(evt->ADC(i)>max){
	max = evt->ADC(i);
	}
	}
      int baseline = (totaladc-max)/31;
      std::cout<<" Maximum ADC value:"<<max<<std::endl;
      std::cout<<" Average without Max value:"<<baseline<<std::endl;
      sbndaq::sendMetric("CRT_board", readout_number_str, "MaxADCValue", max, 0, artdaq::MetricMode::Average);
      sbndaq::sendMetric("CRT_board", readout_number_str, "baseline", baseline, 0, artdaq::MetricMode::Average);

//Burke Change: named time, time0 and created variables, time1, ADC
      int time0 = evt->Time_TS0();
      int time_1 = evt->Time_TS1();
      

//Per board front end metric group
      sbndaq::sendMetric("CRT_board", readout_number_str, "TS0", time0, 0, artdaq::MetricMode::LastPoint);
      sbndaq::sendMetric("CRT_board", readout_number_str, "TS1", time_1, 0, artdaq::MetricMode::LastPoint);
 


//ADC Analysis metric group


    }//end loop over events in a fragment
  
  }//end loop over fragments


}

DEFINE_ART_MODULE(sbndaq::BernCRTZMQAna)
//this is where the name is specified
