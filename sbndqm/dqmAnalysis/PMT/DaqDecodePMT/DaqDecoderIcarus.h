#ifndef DaqDecoderIcarus_h
#define DaqDecoderIcarus_h
////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderIcarus
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderIcarus.h
//
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "lardataobj/RawData/RawDigit.h"
#include "artdaq-core/Data/Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/PhysCrateFragment.hh"
#include "canvas/Utilities/InputTag.h"

//#include "sbnddaq-datatypes/Overlays/NevisTPCFragment.hh"

//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TFile.h"
#include "TVirtualFFT.h"

//"art" includes (canvas, and gallery)
//#include "gallery/Event.h"
//#include "gallery/ValidHandle.h"
#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"

//#include "icarus-artdaq-core/Overlays/PhysCrateFragment.hh"

//#include "WaveformPropertiesAlg.h"

/*
  * The Decoder module takes as input "NevisTPCFragments" and
  * outputs raw::RawDigits. It also handles in and all issues
  * with the passed in header and fragments (or at least it will).
*/

namespace daq {
  class DaqDecoderIcarusPMT;
}


class daq::DaqDecoderIcarusPMT : public art::EDProducer {
public:
  explicit DaqDecoderIcarusPMT(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  DaqDecoderIcarusPMT(DaqDecoderIcarusPMT const &) = delete;
  DaqDecoderIcarusPMT(DaqDecoderIcarusPMT &&) = delete;
  DaqDecoderIcarusPMT & operator = (DaqDecoderIcarusPMT const &) = delete;
  DaqDecoderIcarusPMT & operator = (DaqDecoderIcarusPMT &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;

  // get checksum from a Icarus fragment
  //static uint32_t compute_checksum(sbnddaq::NevisTPCFragment &fragment);
  static uint32_t compute_checksum(icarus::PhysCrateFragment &fragment);

private:
  class Config {
    public:
    int wait_sec;
    int wait_usec;
    bool produce_header;
    bool produce_metadata;
    bool baseline_calc;
    unsigned n_mode_skip;
    bool subtract_pedestal;

    unsigned channel_per_slot;
    unsigned min_slot_no;

    // for converting frame time into timestamp
    unsigned timesize;
    double frame_to_dt;

    Config(fhicl::ParameterSet const & p);
  };

  // process an individual fragment inside an art event
  void process_fragment(art::Event &event, const artdaq::Fragment &frag,
    std::unique_ptr<std::vector<raw::RawDigit>> &product_collectionf);


  // Gets the WIRE ID of the channel. This wire id can be then passed
  // to the Lariat geometry.
  //raw::ChannelID_t get_wire_id(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id);

  // whether the given nevis readout channel is mapped to a wire
  //bool is_mapped_channel(const sbnddaq::NevisTPCHeader *header, uint16_t nevis_channel_id);

  art::InputTag _tag;
  Config _config;
  // keeping track of incrementing numbers
  uint32_t _last_event_number;
  uint32_t _last_trig_frame_number;
};

#endif /* DaqDecoderIcarus_h */
