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
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
#include "sbndqm/dqmAnalysis/ChannelMapping/IICARUSChannelMap.h"

#include "lardataobj/RawData/OpDetWaveform.h"
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"

#include "DaqDecoderIcarusPMT.h"


daq::DaqDecoderIcarusPMT::DaqDecoderIcarusPMT(Parameters const & params)
  : art::EDProducer(params)
  , m_input_tags{ params().FragmentsLabels() }

{
  
  // Output data products
  produces<std::vector<raw::OpDetWaveform>>();
  produces<std::vector<pmtAnalysis::PMTDigitizerInfo>>();

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


std::vector<art::Handle<artdaq::Fragments>> daq::DaqDecoderIcarusPMT::readHandles( art::Event & event ){

  // Normally or the CAENV1730 or the ContainerCAENV1730 are full
  // We return the one of the two that is full  

  std::vector<art::Handle<artdaq::Fragments>> handles;
  art::InputTag this_input_tag; 

  for( art::InputTag const& input_tag : m_input_tags ) {

    art::Handle<artdaq::Fragments> thisHandle;

    if( !event.getByLabel<std::vector<artdaq::Fragment>>(input_tag, thisHandle) ) continue;
    if( !thisHandle.isValid() || thisHandle->empty() ) continue;

    handles.push_back( thisHandle );

  }

  return handles;

} 


artdaq::Fragments daq::DaqDecoderIcarusPMT::readFragments( std::vector<art::Handle<artdaq::Fragments>> handles ) {

  // Hopefully-not-too-sloppy-code to create the fragment list out of the container  
  artdaq::FragmentPtrs containerFragments;
  artdaq::Fragments fragments;
  
  for( const auto& handle : handles ) {
    
    if( handle->front().type() == artdaq::Fragment::ContainerFragmentType ) {
      
      for ( auto const& cont : *handle ) {
	
	artdaq::ContainerFragment contf(cont);
	
	if( contf.fragment_type() != sbndaq::detail::FragmentType::CAENV1730 ) { break; }
	
	for( size_t ii=0; ii < contf.block_count(); ii++ ) {
	  
	  // A bit unnecessary for this case 
	  containerFragments.push_back(contf[ii]);
	  fragments.push_back( *containerFragments.back() );
	  
	}

      } 
      
    }
    
    else {
      
      if ( handle->front().type() != sbndaq::detail::FragmentType::CAENV1730  ) { break; }
	
      for( auto frag : *handle ) {
	  
	fragments.emplace_back( frag );
	  	
      }

    } // end if container

  } // end for handles 
  

  return fragments;

}


void daq::DaqDecoderIcarusPMT::processFragment( const artdaq::Fragment &artdaqFragment ) {

  size_t const fragment_id = artdaqFragment.fragmentID();
  size_t const eff_fragment_id = fragment_id & 0x0fff;

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

  artdaq::Fragment::timestamp_t const fragmentTimestamp = artdaqFragment.timestamp(); 
  unsigned int const time_tag =  header.triggerTimeTag;

  auto const [ chDataMap, nEnabledChannels ] = setBitIndices<16U>(enabledChannels);
  const uint16_t* data_begin = reinterpret_cast<const uint16_t*>(artdaqFragment.dataBeginBytes() + sizeof(sbndaq::CAENV1730EventHeader));

  std::vector<uint16_t> wvfm(nSamplesPerChannel);
  float temperature = 0;


  for( size_t digitizerChannel=0; digitizerChannel<nChannelsPerBoard; digitizerChannel++ ) {

    std::size_t const pmtID = digitizerChannel+nChannelsPerBoard*eff_fragment_id;

    std::size_t const channelPosInData = chDataMap[digitizerChannel];
    if (channelPosInData >= nEnabledChannels) continue; // not enabled
    std::size_t const ch_offset = channelPosInData * nSamplesPerChannel;

    std::copy_n(data_begin + ch_offset, nSamplesPerChannel, wvfm.begin());

    fOpDetWaveformCollection->emplace_back(fragmentTimestamp, pmtID, wvfm);

    temperature += float( metafrag.chTemps[digitizerChannel] );

  }

  if (nEnabledChannels > 0) temperature /= nEnabledChannels;
  else temperature = -1.0f; // invalid temperature

  fPMTDigitizerInfoCollection->emplace_back( eff_fragment_id, time_tag, fragmentTimestamp, temperature );

}


void daq::DaqDecoderIcarusPMT::produce(art::Event & event)
{

  // Make the list of the input fragments 

  try {

    auto fragmentHandles = readHandles( event );

    auto fragments = readFragments( fragmentHandles );

    // initialize the data product 
    fOpDetWaveformCollection = OpDetWaveformCollectionPtr(new OpDetWaveformCollection);
    fPMTDigitizerInfoCollection = PMTDigitizerInfoCollectionPtr(new PMTDigitizerInfoCollection);
    
    if ( fragments.size() > 0 ){
 
      for( auto const & fragment : fragments ) {   
  
          processFragment( fragment );
   
      }
    
    }
    
    else {

       mf::LogError("DaqDecoderIcarus") 
	 << "No fragments found\n" << '\n';
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

 
