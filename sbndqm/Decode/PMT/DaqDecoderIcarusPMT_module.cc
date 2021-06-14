////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderIcarus
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderIcarus.cxx
//
// mailto ascarpel@bnl.gov ( with great help of G. Petrillo et al .. )
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "cetlib_except/exception.h"

#include <memory>
#include <iostream>
#include <stdlib.h>
#include <chrono>
#include <thread>

#include "art/Framework/Core/ModuleMacros.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/PhysCrateFragment.hh"
#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh" // sbndaq::FragmentType
#include "sbndqm/dqmAnalysis/ChannelMapping/IICARUSChannelMap.h"

#include "lardataobj/RawData/OpDetWaveform.h"

#include "DaqDecoderIcarusPMT.h"


daq::DaqDecoderIcarusPMT::DaqDecoderIcarusPMT(Parameters const & params)
  : art::EDProducer(params)
  , m_input_tags{ params().FragmentsLabels() }
  , fChannelMap{ *(art::ServiceHandle<icarusDB::IICARUSChannelMap const>{}) }

{
  
  // Output data products
	produces<std::vector<raw::OpDetWaveform>>();
  produces<std::vector<pmtAnalysis::PMTDigitizerInfo>>();

  std::unique_ptr<std::vector<raw::OpDetWaveform>> product_collection(new std::vector<raw::OpDetWaveform>());

}


template <std::size_t NBits, typename T>
constexpr std::pair<std::array<std::size_t, NBits>, std::size_t>
daq::DaqDecoderIcarusPMT::setBitIndices(T value) noexcept {
  
  std::pair<std::array<std::size_t, NBits>, std::size_t> res;
  auto& [ indices, nSetBits ] = res;
  for (std::size_t& index: indices) {
    index = (value & 1)? nSetBits++: NBits;
    value >>= 1;
  } // for
  return res;
  
} 


std::vector<artdaq::Fragment> daq::DaqDecoderIcarusPMT::readFragments( art::Event & event ){

  art::Handle<std::vector<artdaq::Fragment>> handle;
  art::InputTag this_input_tag; 

  for( art::InputTag const& input_tag : m_input_tags ) {

    art::Handle<std::vector<artdaq::Fragment>> thisHandle;

    if( !event.getByLabel<std::vector<artdaq::Fragment>>(input_tag, thisHandle) ) continue;
    if( !thisHandle.isValid() || thisHandle->empty() ) continue;

    handle = thisHandle;

  }

  return *handle;

} 


void daq::DaqDecoderIcarusPMT::processFragment( const artdaq::Fragment &artdaqFragment ) {

  size_t const fragment_id = artdaqFragment.fragmentID();
  size_t const eff_fragment_id = artdaqFragment.fragmentID() & 0x0fff;

  sbndaq::CAENV1730Fragment fragment(artdaqFragment);
  sbndaq::CAENV1730FragmentMetadata metafrag = *fragment.Metadata();
  sbndaq::CAENV1730Event evt = *fragment.Event();
  sbndaq::CAENV1730EventHeader header = evt.Header;

  size_t nChannelsPerBoard = metafrag.nChannels;

  uint32_t ev_size_quad_bytes         = header.eventSize;
  uint32_t evt_header_size_quad_bytes = sizeof(sbndaq::CAENV1730EventHeader)/sizeof(uint32_t);
  uint32_t data_size_double_bytes     = 2*(ev_size_quad_bytes - evt_header_size_quad_bytes);
  uint32_t nSamplesPerChannel         = data_size_double_bytes/nChannelsPerBoard;
  uint16_t const enabledChannels      = header.ChannelMask();

  //artdaq::Fragment::timestamp_t const fragmentTimestamp = artdaqFragment.timestamp(); << uncomment me

  unsigned int const time_tag =  header.triggerTimeTag;

  //size_t boardId = nChannelsPerBoard * eff_fragment_id; << uncomment me

  auto const [ chDataMap, nEnabledChannels ] = setBitIndices<16U>(enabledChannels);

  const uint16_t* data_begin = reinterpret_cast<const uint16_t*>(artdaqFragment.dataBeginBytes() + sizeof(sbndaq::CAENV1730EventHeader));

  if (fChannelMap.hasPMTDigitizerID(eff_fragment_id)) {

      const icarusDB::DigitizerChannelChannelIDPairVec& digitizerChannelVec
          = fChannelMap.getChannelIDPairVec(eff_fragment_id);


      std::vector<uint16_t> wvfm(nSamplesPerChannel);


      for(auto const [ digitizerChannel, channelID ]: digitizerChannelVec) {

        std::size_t const channelPosInData = chDataMap[digitizerChannel];
        std::size_t const ch_offset = channelPosInData * nSamplesPerChannel;

        std::copy_n(data_begin + ch_offset, nSamplesPerChannel, wvfm.begin());

        fOpDetWaveformCollection->emplace_back(time_tag, channelID, wvfm);

      }

  }

  else {
        
    mf::LogError("DaqDecoderIcarus")
      << "*** PMT could not find channel information for fragment: "
        << fragment_id;

  }

  //uint32_t temperature = 0;

  //metafrag.chTemps

  //PMTDigitizerInfo dgtz( eff_fragment_id, time_tag, temperature )

  //v_digitizer_info.push_back(  )


}


void daq::DaqDecoderIcarusPMT::produce(art::Event & event)
{

  // Make the list of the input fragments 

  try {

    auto fragments = readFragments( event );

    // initialize the data product 
    fOpDetWaveformCollection = OpDetWaveformCollectionPtr(new OpDetWaveformCollection);
    fPMTDigitizerInfoCollection = PMTDigitizerInfoCollectionPtr(new PMTDigitizerInfoCollection);

    for( auto const & fragment : fragments ) { 

      switch( fragment.type() ) {
        
        case sbndaq::FragmentType::CAENV1730:
          processFragment( fragment );
        
        case artdaq::Fragment::ContainerFragmentType:
          processFragment( fragment );
       
        default: 

          throw cet::exception("DaqDecoderIcarusPMT")
            << "Unexpected PMT data product fragment type \n";
      }

    }

    // Place the data product in the event stream
    event.put(std::move(fOpDetWaveformCollection));
    event.put(std::move(fPMTDigitizerInfoCollection));

  }

  catch( cet::exception const& e ){

    mf::LogError("DaqDecoderIcarus") 
      << "Error while attempting to decode PMT data:\n" << e.what() << '\n';

  }

}

DEFINE_ART_MODULE(daq::DaqDecoderIcarusPMT)

 

