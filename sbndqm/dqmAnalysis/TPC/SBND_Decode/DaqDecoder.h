#ifndef DaqDecoder_h
#define DaqDecoder_h
////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoder
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoder.h
//
// Generated at Thu Feb  8 16:41:18 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_01_03.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "lardataobj/RawData/RawDigit.h"
#include "artdaq-core/Data/Fragment.hh"
#include "canvas/Utilities/InputTag.h"

#include "sbnddaq-datatypes/Overlays/NevisTPCFragment.hh"

#include "../HeaderData.hh"
#include "../VSTChannelMap.hh"

/*
  * The Decoder module takes as input "NevisTPCFragments" and
  * outputs raw::RawDigits. It also handles in and all issues
  * with the passed in header and fragments (or at least it will).
*/

namespace daq {
  class DaqDecoder;
}


class daq::DaqDecoder : public art::EDProducer {
public:
  explicit DaqDecoder(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  DaqDecoder(DaqDecoder const &) = delete;
  DaqDecoder(DaqDecoder &&) = delete;
  DaqDecoder & operator = (DaqDecoder const &) = delete;
  DaqDecoder & operator = (DaqDecoder &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;

  // get checksum from a Nevis fragment
  static uint32_t compute_checksum(sbnddaq::NevisTPCFragment &fragment);

private:
  class Config {
    public:
    int wait_sec;
    int wait_usec;
    bool produce_header;
    bool produce_metadata;
    bool baseline_calc;
    bool validate_header;
    unsigned n_mode_skip;
    bool calc_checksum;
    bool subtract_pedestal;

    // for converting nevis frame time into timestamp
    unsigned timesize;
    unsigned frame_to_dt;

    bool v_checksum;
    bool v_event_no;
    bool v_slot_no;
    bool v_adc_count_non_zero;
    bool v_inc_event_no;
    bool v_inc_trig_frame_no;
    Config(fhicl::ParameterSet const & p);
  };

  // process an individual fragment inside an art event
  void process_fragment(art::Event &event, const artdaq::Fragment &frag,
    std::unique_ptr<std::vector<raw::RawDigit>> &product_collection,
    std::unique_ptr<std::vector<daqAnalysis::HeaderData>> &header_collection);

  // validate Nevis header
  void validate_header(const daqAnalysis::HeaderData &header);

  // handle to the channel map service
  art::ServiceHandle<daqAnalysis::VSTChannelMap> _channel_map;

  // Gets the WIRE ID of the channel. This wire id can be then passed
  // to the Lariat geometry.
  raw::ChannelID_t get_wire_id(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id);

  // whether the given nevis readout channel is mapped to a wire
  bool is_mapped_channel(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id);

  art::InputTag _tag;
  Config _config;
  // keeping track of incrementing numbers
  uint32_t _last_event_number;
  uint32_t _last_trig_frame_number;
};

#endif /* DaqDecoder_h */
