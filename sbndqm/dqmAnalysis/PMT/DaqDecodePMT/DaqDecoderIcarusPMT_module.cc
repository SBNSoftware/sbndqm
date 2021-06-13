////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderIcarus
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderIcarus.cxx
//
////////////////////////////////////////////////////////////////////////

#include "DaqDecoderIcarus.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>
#include <iostream>
#include <stdlib.h>
#include <chrono>
#include <thread>

#include "art/Framework/Core/ModuleMacros.h"

#include "artdaq-core/Data/Fragment.hh"

#include "sbndaq-artdaq-core/Overlays/ICARUS/PhysCrateFragment.hh"
#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
#include "artdaq-core/Data/Fragment.hh"

#include "../PMTWaveform.hh"

DEFINE_ART_MODULE(daq::DaqDecoderIcarusPMT)

daq::DaqDecoderIcarusPMT::DaqDecoderIcarusPMT(fhicl::ParameterSet const & param): 
  art::EDProducer{param},
  _tag(param.get<std::string>("raw_data_label", "daq"),param.get<std::string>("fragment_type_label", "CAENV1730")),
  _config(param),
  _last_event_number(0),
  _last_trig_frame_number(0)
{
  
	produces<std::vector<pmtAnalysis::PMTWaveform>>();

}

daq::DaqDecoderIcarusPMT::Config::Config(fhicl::ParameterSet const & param) {

  double wait_time = param.get<double>("wait_time", -1 /* units of seconds */);
  wait_sec = (int) wait_time;
  wait_usec = (int) (wait_time / 1000000.);
  
}

void daq::DaqDecoderIcarusPMT::produce(art::Event & event)
{


  if (_config.wait_sec >= 0) {
    std::this_thread::sleep_for(std::chrono::seconds(_config.wait_sec) + std::chrono::microseconds(_config.wait_usec));
  }
  
  std::unique_ptr<std::vector<pmtAnalysis::PMTWaveform>> product_collection(new std::vector<pmtAnalysis::PMTWaveform>());
  

  /************************************************************************************************/
  art::Handle< std::vector<artdaq::Fragment> > rawFragHandle;
  event.getByLabel("daq","CAENV1730", rawFragHandle);
  if (rawFragHandle.isValid()) {

    std::cout << "######################################################################\n";
    std::cout << "Run " << event.run() << ", subrun " << event.subRun();

 for (size_t idx = 0; idx < rawFragHandle->size(); ++idx) { /*loop over the fragments*/
      //--use this fragment as a reference to the same data
      
      size_t fragment_id = (*rawFragHandle)[idx].fragmentID();

      const auto& frag((*rawFragHandle)[idx]); 
      sbndaq::CAENV1730Fragment bb(frag);
      auto const* md = bb.Metadata();
      sbndaq::CAENV1730Event const* event_ptr = bb.Event();
      sbndaq::CAENV1730EventHeader header = event_ptr->Header;
     
      std::vector< std::vector<uint16_t> >  fWvfmsVec;
      size_t nChannels = md->nChannels;
      
      uint32_t ev_size_quad_bytes = header.eventSize;
      uint32_t evt_header_size_quad_bytes = sizeof(sbndaq::CAENV1730EventHeader)/sizeof(uint32_t);
      uint32_t data_size_double_bytes = 2*(ev_size_quad_bytes - evt_header_size_quad_bytes);
      uint32_t wfm_length = data_size_double_bytes/nChannels;

      const uint16_t* data_begin = reinterpret_cast<const uint16_t*>(frag.dataBeginBytes() 
                                 + sizeof(sbndaq::CAENV1730EventHeader));

      const uint16_t* value_ptr =  data_begin;
      size_t ch_offset = 0;
      uint16_t value;


      for (size_t i_ch=0; i_ch<nChannels; ++i_ch){
      
      ch_offset = i_ch * wfm_length;

      size_t i_daq = i_ch + nChannels*fragment_id;
      
      pmtAnalysis::PMTWaveform my_wf(0.00, i_daq, wfm_length);
      my_wf.resize(wfm_length);
      
      for(size_t i_t=0; i_t<wfm_length; ++i_t){ 

          value_ptr = data_begin + ch_offset + i_t; 
          value = *(value_ptr);
          my_wf[i_t] = value;

        }
 
      product_collection->push_back(my_wf);

      }// end loop over channels

     } // end loop over fragments

}

event.put(std::move(product_collection));

}

/*
void daq::DaqDecoderIcarusPMT::process_fragment(art::Event &event, const artdaq::Fragment &frag, 
  std::unique_ptr<std::vector<raw::RawDigit>> &product_collection) {

  icarus::PhysCrateFragment fragment(frag);
	std::cout << " n boards " << fragment.nBoards() << std::endl;

for(size_t i_b=0; i_b < fragment.nBoards(); i_b++){

	for(size_t i_ch=0; i_ch < fragment.nChannelsPerBoard(); ++i_ch){

	  //raw::ChannelID_t channel_num = (i_ch & 0xff ) + (i_b << 8);
           raw::ChannelID_t channel_num = i_ch+i_b*64;
	  raw::RawDigit::ADCvector_t wvfm(fragment.nSamplesPerChannel());
	  for(size_t i_t=0; i_t < fragment.nSamplesPerChannel(); ++i_t) {
	    wvfm[i_t] = fragment.adc_val(i_b,i_ch,i_t);
}
      		product_collection->emplace_back(channel_num,fragment.nSamplesPerChannel(),wvfm);

	}//loop over channels

      }//loop over boards
      std::cout << "Total number of channels added is " << product_collection->size() << std::endl;


  
}
*/



 

