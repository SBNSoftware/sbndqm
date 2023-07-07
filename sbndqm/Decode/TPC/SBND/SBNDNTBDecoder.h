#ifndef SBNDNTBDecoder_h
#define SBNDNTBDecoder_h

////////////////////////////////////////////////////////////////////////                                                                     
// Class:       SBNDNTBDecoder                                                                                                               
// Plugin Type: producer (art v2_09_06)                                                                                                     // Generated at June 28 2023 by Daisy Kalra using cetskelgen                                                                                 
////////////////////////////////////////////////////////////////////////      

#include "art/Framework/Core/EDProducer.h"
#include "lardataobj/RawData/RawDigit.h"
#include "artdaq-core/Data/Fragment.hh"
#include "canvas/Utilities/InputTag.h"

#include "/home/nfs/sbnd/DAQ_DevAreas/DAQ_TPC_NTB_May2023/srcs/sbndaq_artdaq_core/sbndaq-artdaq-core/Overlays/SBND/NevisTBFragment.hh"

#include "../NTBHeaderData.hh"

namespace daq {
  class SBNDNTBDecoder;
}

class daq::SBNDNTBDecoder : public art::EDProducer {
 public:

  explicit SBNDNTBDecoder(fhicl::ParameterSet const & p);

  SBNDNTBDecoder(SBNDNTBDecoder const &) = delete;
  SBNDNTBDecoder(SBNDNTBDecoder &&) = delete;
  SBNDNTBDecoder & operator = (SBNDNTBDecoder const &) = delete;
  SBNDNTBDecoder & operator = (SBNDNTBDecoder &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;

  // get checksum from a Nevis fragment
  //  static uint32_t compute_checksum(sbndaq::NevisTPCFragment &fragment);


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

    //unsigned channel_per_slot;
    //unsigned min_slot_no;

    // for converting nevis frame time into timestamp
    //unsigned timesize;
    //double frame_to_dt;

    Config(fhicl::ParameterSet const & p);
  };

  void process_fragment(art::Event &event, const artdaq::Fragment &frag,
			std::unique_ptr<std::vector<raw::RawDigit>> &product_collection,
			std::unique_ptr<std::vector<ntbAnalysis::NTBHeaderData>> &header_collection);


  // build a HeaderData object from the Nevis Header
  ntbAnalysis::NTBHeaderData Fragment2NTBHeaderData(art::Event &event, const artdaq::Fragment &frag);

  art::InputTag _tag;
  Config _config;
  // keeping track of incrementing numbers
  uint32_t _last_event_number;
  uint32_t _last_trig_frame_number;
};

#endif
