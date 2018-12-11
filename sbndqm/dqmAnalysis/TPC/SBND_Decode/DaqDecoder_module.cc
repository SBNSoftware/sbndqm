////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoder
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoder.cxx
//
// Generated at Thu Feb  8 16:41:18 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_01_03.
////////////////////////////////////////////////////////////////////////

#include "DaqDecoder.h"
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

#include "sbnddaq-datatypes/Overlays/NevisTPCFragment.hh"
#include "sbnddaq-datatypes/NevisTPC/NevisTPCTypes.hh"
#include "sbnddaq-datatypes/NevisTPC/NevisTPCUtilities.hh"

#include "../HeaderData.hh"
#include "../VSTChannelMap.hh"
#include "../Mode.hh"

DEFINE_ART_MODULE(daq::DaqDecoder)

// constructs a header data object from a nevis header
// construct from a nevis header
daqAnalysis::HeaderData Fragment2HeaderData(art::Event &event, const artdaq::Fragment &frag, unsigned frame_to_dt=1, unsigned timesize=1, bool calc_checksum=false) {
  sbnddaq::NevisTPCFragment fragment(frag);

  const sbnddaq::NevisTPCHeader *raw_header = fragment.header();
  daqAnalysis::HeaderData ret;

  ret.crate = raw_header->getFEMID();
  ret.slot = raw_header->getSlot();
  ret.event_number = raw_header->getEventNum();
  ret.frame_number = raw_header->getFrameNum();
  ret.checksum = raw_header->getChecksum();
  
  if (calc_checksum) {
    ret.computed_checksum = daq::DaqDecoder::compute_checksum(fragment);
  }
  else {
    ret.computed_checksum = 0;
  }

  ret.adc_word_count = raw_header->getADCWordCount();
  ret.trig_frame_number = raw_header->getTrigFrame();
  
  ret.frame_time = ret.frame_number * frame_to_dt;
  ret.trig_frame_time = ret.trig_frame_number * frame_to_dt;
  ret.two_mhzsample = raw_header->get2mhzSample();

  // run id stuff
  ret.run_no = event.run();
  ret.sub_run_no = event.subRun();
  ret.art_event_no = event.event();

  // raw header stuff
  ret.id_and_slot_word = raw_header->id_and_slot;
  ret.word_count_word = raw_header->word_count;
  ret.event_num_word = raw_header->event_num;
  ret.frame_num_word = raw_header->frame_num;
  ret.checksum_word = raw_header->checksum;
  ret.trig_frame_sample_word = raw_header->trig_frame_sample;

  ret.event_time_stamp_lo = event.time().timeLow();
  ret.event_time_stamp_hi = event.time().timeHigh();

  ret.frag_time_stamp = frag.timestamp();

  // formula for getting unix timestamp from nevis frame number:
  // timestamp = frame_number * (timesize + 1) + trigger_sample

  ret.frame_time_stamp = (ret.frame_number * (timesize + 1) + raw_header->get2mhzSample()) * frame_to_dt;

  return ret;
}

daq::DaqDecoder::DaqDecoder(fhicl::ParameterSet const & param): 
  _channel_map(),
  _tag(param.get<std::string>("raw_data_label", "daq"),param.get<std::string>("fragment_type_label", "NEVISTPC")),
  _config(param),
  _last_event_number(0),
  _last_trig_frame_number(0)
 {
  
  // produce stuff
  produces<std::vector<raw::RawDigit>>();
  if (_config.produce_header) {
    produces<std::vector<daqAnalysis::HeaderData>>();
  }
  if (_config.produce_metadata) {
    produces<std::vector<daqAnalysis::NevisTPCMetaData>>();
  }
}

daq::DaqDecoder::Config::Config(fhicl::ParameterSet const & param) {
  // amount of time to wait in between processing events
  // useful for debugging redis
  double wait_time = param.get<double>("wait_time", -1 /* units of seconds */);
  wait_sec = (int) wait_time;
  wait_usec = (int) (wait_time / 1000000.);
  // whether to calcualte the pedestal (and set it in SetPedestal())
  baseline_calc = param.get<bool>("baseline_calc", false);
  // whether to put headerinfo in the art root file
  produce_header = param.get<bool>("produce_header", false);
  // whether to put NevisTPCMetaData in the art root file
  produce_metadata = param.get<bool>("produce_metadata", false);
  // whether to check if Header looks good and print out error info
  validate_header = param.get<bool>("validate_header", false);
  // how many adc values to skip in mode/pedestal finding
  n_mode_skip = param.get<unsigned>("n_mode_skip", 1);
  // whether to verify checksum
  calc_checksum = param.get<bool>("calc_checksum", false);
  // whether to subtract pedestal
  subtract_pedestal = param.get<bool>("subtract_pedestal", false);

  // nevis readout window length
  timesize = param.get<unsigned>("timesize", 1);

  // nevis tick length (for timestamp)
  // should be 1/(2MHz) = 0.5mus
  frame_to_dt = param.get<unsigned>("frame_to_dt", 1);

  // turning off various config stuff
  v_checksum = param.get<bool>("v_check_checksum", true);
  v_event_no = param.get<bool>("v_crosscheck_event_no", true);
  v_slot_no = param.get<bool>("v_slot_no", true);
  v_adc_count_non_zero = param.get<bool>("v_adc_count_non_zero", true);
  v_inc_event_no = param.get<bool>("v_inc_event_no", true);
  v_inc_trig_frame_no = param.get<bool>("v_inc_trig_frame_no", true);
}

void daq::DaqDecoder::produce(art::Event & event)
{
  if (_config.wait_sec >= 0) {
    std::this_thread::sleep_for(std::chrono::seconds(_config.wait_sec) + std::chrono::microseconds(_config.wait_usec));
  }
  auto const& daq_handle = event.getValidHandle<artdaq::Fragments>(_tag);
  
  // storage for waveform
  std::unique_ptr<std::vector<raw::RawDigit>> product_collection(new std::vector<raw::RawDigit>);
  // storage for header info
  std::unique_ptr<std::vector<daqAnalysis::HeaderData>> header_collection(new std::vector<daqAnalysis::HeaderData>);

  for (auto const &rawfrag: *daq_handle) {
    process_fragment(event, rawfrag, product_collection, header_collection);
  }

  event.put(std::move(product_collection));

  if (_config.produce_metadata) {
    // put metadata in event
    std::unique_ptr<std::vector<daqAnalysis::NevisTPCMetaData>> metadata_collection(new std::vector<daqAnalysis::NevisTPCMetaData>);
    for (auto const &header: *header_collection) {
      metadata_collection->emplace_back(header);
    }
    event.put(std::move(metadata_collection));
  }

  if (_config.produce_header) {
    event.put(std::move(header_collection));
  }

}

bool daq::DaqDecoder::is_mapped_channel(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id) {
  // rely on ChannelMap for implementation
  return _channel_map->IsMappedChannel(nevis_channel_id, header->getSlot(), header->getFEMID());
}

raw::ChannelID_t daq::DaqDecoder::get_wire_id(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id) {
  // rely on ChannelMap for implementation
  return _channel_map->Channel2Wire(nevis_channel_id, header->getSlot(), header->getFEMID());
}

void daq::DaqDecoder::process_fragment(art::Event &event, const artdaq::Fragment &frag, 
  std::unique_ptr<std::vector<raw::RawDigit>> &product_collection,
  std::unique_ptr<std::vector<daqAnalysis::HeaderData>> &header_collection) {

  // convert fragment to Nevis fragment
  sbnddaq::NevisTPCFragment fragment(frag);

  std::unordered_map<uint16_t,sbnddaq::NevisTPC_Data_t> waveform_map;
  size_t n_waveforms = fragment.decode_data(waveform_map);
  (void)n_waveforms;

  if (_config.produce_header || _config.validate_header || _config.produce_metadata /*make header info to convert into metdata later*/) {
    auto header_data = Fragment2HeaderData(event, frag, _config.frame_to_dt, _config.timesize, _config.calc_checksum);
    if (_config.produce_header || _config.produce_metadata) {
      // Construct HeaderData from the Nevis Header and throw it in the collection
      header_collection->push_back(header_data);
    }
    if (_config.validate_header) {
      validate_header(header_data);
    }
  }

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
}
void daq::DaqDecoder::validate_header(const daqAnalysis::HeaderData &header) {
  bool printed = false;
  if (_config.v_checksum && _config.calc_checksum && header.checksum != header.computed_checksum) {
   unsigned checksum = header.checksum;
   unsigned computed_checksum = header.computed_checksum;
    mf::LogError("Bad Header") << std::hex << "computed checksum " << 
      checksum << " does not match firmware checksum " << computed_checksum ;
    printed = true;
  }
  if (_config.v_event_no && header.art_event_no != header.event_number) {
    unsigned art_event_no = header.art_event_no;
    unsigned frag_event_no = header.event_number;
    mf::LogError("Bad Header") << "Art event number " << art_event_no <<
      " does not match firmware event number " << frag_event_no;
    printed = true;
  }

  // negative overflow will wrap around and also be large than n_fem_per_crate,
  // so this covers both the case where the slot id is too big and too small
  if (!_channel_map->IsGoodSlot(header.slot)) {
    unsigned slot = header.slot;
    mf::LogError("Bad Header") << "Bad slot index: " << slot;
    printed = true;
  }

  if (_config.v_adc_count_non_zero && header.adc_word_count == 0) {
    unsigned fem_ind = _channel_map->SlotIndex(header);
    unsigned crate = header.crate;
    unsigned slot = header.slot;
    mf::LogWarning("Bad Header") << "ADC Word Count in crate " << crate << ", slot " << 
      slot << ", fem ID " << fem_ind << "is 0" ;
    printed = true;
  }
  if (_config.v_inc_event_no && header.event_number < _last_event_number) {
    unsigned event_number = header.event_number;
    mf::LogError("Bad Header") << "Non incrementing event numbers. Last event number: " << 
      _last_event_number << ". This event number: " << event_number ;
    printed = true;
  }
  if (_config.v_inc_trig_frame_no && header.trig_frame_number < _last_trig_frame_number) {
    unsigned trig_frame_number = header.trig_frame_number;
    mf::LogError("Bad Header") << "Non incrementing trig frame numbers. Last trig frame: " << 
      _last_trig_frame_number << " This trig frame: " << trig_frame_number ;
    printed = true;
  }
  if (printed) {
     mf::LogInfo("Bad Header") << "Header Info:\n" <<  header.Print() ;
  }
  // store numbers for next time
  _last_event_number = header.event_number;
  _last_trig_frame_number = header.trig_frame_number;
  return; 
}

// Computes the checksum, given a nevis tpc header
// Ideally this would be in sbnddaq-datatypes, but it's not and I can't
// make changes to it, so put it here for now
//
// Also note that this only works for uncompressed data
uint32_t daq::DaqDecoder::compute_checksum(sbnddaq::NevisTPCFragment &fragment) {
  uint32_t checksum = 0;

  const sbnddaq::NevisTPC_ADC_t* data_ptr = fragment.data();
  // RETURN VALUE OF getADCWordCount IS OFF BY 1
  size_t n_words = fragment.header()->getADCWordCount() + 1;

  for (size_t word_ind = 0; word_ind < n_words; word_ind++) {
    const sbnddaq::NevisTPC_ADC_t* word_ptr = data_ptr + word_ind;
    checksum += *word_ptr;
  }
  // only first 6 bytes of checksum are used
  return checksum & 0xFFFFFF;

}



 

