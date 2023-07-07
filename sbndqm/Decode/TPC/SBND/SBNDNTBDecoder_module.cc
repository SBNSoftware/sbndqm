////////////////////////////////////////////////////////////////////////
// Class:       SBNDNTBDecoder
// Plugin Type: producer (art v2_09_06)
// File:        SBNDNTBDecoder.cxx
//
// Generated at June 28 2023 by Daisy Kalra using cetskelgen
// 
////////////////////////////////////////////////////////////////////////

#include "SBNDNTBDecoder.h"
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

#include "lardataobj/RawData/RawDigit.h"

#include "sbndaq-artdaq-core/Overlays/SBND/NevisTBFragment.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/NevisTB_dataFormat.hh"

//#include "sbndaq-artdaq-core/Overlays/SBND/NevisTPCFragment.hh"
//#include "sbndaq-artdaq-core/Overlays/SBND/NevisTPC/NevisTPCTypes.hh"
//#include "sbndaq-artdaq-core/Overlays/SBND/NevisTPC/NevisTPCUtilities.hh"

#include "sbndqm/Decode/TPC/NTBHeaderData.hh"
#include "sbndqm/Decode/Mode/Mode.hh"

DEFINE_ART_MODULE(daq::SBNDNTBDecoder)

// constructs a header data object from a nevis header
// construct from a nevis header
ntbAnalysis::NTBHeaderData daq::SBNDNTBDecoder::Fragment2NTBHeaderData(art::Event &event, const artdaq::Fragment &frag) {
  sbndaq::NevisTBFragment fragment(frag);
 
  const sbndaq::NevisTrigger_Header *raw_header = fragment.header();
  //  const ntbAnalysis::NTBHeaderData *raw_header = fragment.header();  
 
 ntbAnalysis::NTBHeaderData ret;

  ret.sample_ntb = raw_header->getSampleNumber();
  ret.frame_ntb = raw_header->getFrame();
  ret.event_ntb = raw_header->getTriggerNumber();

  return ret;
}


daq::SBNDNTBDecoder::SBNDNTBDecoder(fhicl::ParameterSet const & param): 
  art::EDProducer{param},
  _tag(param.get<std::string>("raw_data_label", "daq"),param.get<std::string>("fragment_type_label", "NEVISTB")),
  _config(param),
  _last_event_number(0),
  _last_trig_frame_number(0)
  {

    // produce stuff
    produces<std::vector<raw::RawDigit>>();
    if (_config.produce_header) {
      produces<std::vector<ntbAnalysis::NTBHeaderData>>();
    }
  }
  

daq::SBNDNTBDecoder::Config::Config(fhicl::ParameterSet const & param) {

  // amount of time to wait in between processing events
  // useful for debugging redis
  double wait_time = param.get<double>("wait_time", -1 /* units of seconds */);
  wait_sec = (int) wait_time;
  wait_usec = (int) (wait_time / 1000000.);

  // whether to calcualte the pedestal (and set it in SetPedestal())
  baseline_calc = param.get<bool>("baseline_calc", false);
  // whether to put headerinfo in the art root file
  produce_header = param.get<bool>("produce_header", false);
  // how many adc values to skip in mode/pedestal finding
  n_mode_skip = param.get<unsigned>("n_mode_skip", 1);
  // whether to subtract pedestal
  subtract_pedestal = param.get<bool>("subtract_pedestal", false);

}

void daq::SBNDNTBDecoder::produce(art::Event & event)
{
  if (_config.wait_sec >= 0) {
    std::this_thread::sleep_for(std::chrono::seconds(_config.wait_sec) + std::chrono::microseconds(_config.wait_usec));
  }
  auto const& daq_handle = event.getValidHandle<artdaq::Fragments>(_tag);

  // storage for waveform
  std::unique_ptr<std::vector<raw::RawDigit>> product_collection(new std::vector<raw::RawDigit>);
  // storage for header info
  std::unique_ptr<std::vector<ntbAnalysis::NTBHeaderData>> header_collection(new std::vector<ntbAnalysis::NTBHeaderData>);
  
  for (auto const &rawfrag: *daq_handle) {
    process_fragment(event, rawfrag, product_collection, header_collection);
  }

  event.put(std::move(product_collection));

  if (_config.produce_header) {
    event.put(std::move(header_collection));
  }

}


//Copied from TPC
void daq::SBNDNTBDecoder::process_fragment(art::Event &event, const artdaq::Fragment &frag,
					   std::unique_ptr<std::vector<raw::RawDigit>> &product_collection,
					   std::unique_ptr<std::vector<ntbAnalysis::NTBHeaderData>> &header_collection) {


  // convert fragment to Nevis fragment                                                                                                                               
   sbndaq::NevisTBFragment fragment(frag);

   //  std::unordered_map<uint16_t,sbndaq::NevisTPC_Data_t> waveform_map;
   //size_t n_waveforms = fragment.decode_data(waveform_map);
   //(void)n_waveforms;

  if (_config.produce_header) {
    auto ntbheader_data = Fragment2NTBHeaderData(event, frag);
    if (_config.produce_header || _config.produce_metadata) {
      // Construct HeaderData from the Nevis Header and throw it in the collection                                                                                    
      header_collection->push_back(ntbheader_data);
    }
  }

}
/*
  for (auto waveform: waveform_map) {
    // ignore channels that aren't mapped to a wire                                                                                                                   
    if (!is_mapped_channel(fragment.header(), waveform.first)) continue;

    std::vector<int16_t> raw_digits_waveform;
    raw::ChannelID_t wire_id = get_wire_id(fragment.header(), waveform.first);
    // TODO: is this too expensive an operation?                                                                                                                      
    for (auto digit: waveform.second) {
      raw_digits_waveform.push_back( (int16_t) digit);
    }
    // calculate the mode and set it as the pedestal                                                                                                                  
    if (_config.baseline_calc || _config.subtract_pedestal) {
      int16_t mode = Mode(raw_digits_waveform, _config.n_mode_skip);
      if (_config.subtract_pedestal) {
        for (unsigned i = 0; i < raw_digits_waveform.size(); i++) {
          raw_digits_waveform[i] -= mode;
        }
      }

      // construct the next RawDigit object                                                                                                                           
      product_collection->emplace_back(wire_id, raw_digits_waveform.size(), raw_digits_waveform);

      if (_config.baseline_calc) {
        (*product_collection)[product_collection->size() - 1].SetPedestal( mode);
      }
    }
    // just push back                                                                                                                                                 
    else {
      product_collection->emplace_back(wire_id, raw_digits_waveform.size(), raw_digits_waveform);

    }
  }

  
  */






