////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderSBND
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderSBND.cxx
//
// Lan Nguyen
// Email: vclnguyen1@sheffield.ac.uk
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib_except/exception.h"
#include "canvas/Utilities/InputTag.h"

#include "fhiclcpp/types/Sequence.h"

#include <memory>
#include <vector>
#include <array>
#include <utility> // std::pair, std::move()
#include <stdlib.h>
#include <cassert>

#include "art/Framework/Core/ModuleMacros.h"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/PhysCrateFragment.hh"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"


namespace daq 
{

  class DaqDecoderSBNDPMT : public art::EDProducer 
  {

    public:

      struct Config 
      {

        using Name = fhicl::Name;

        using Comment = fhicl::Comment;

        fhicl::Sequence<art::InputTag> FragmentsLabels {
          Name("FragmentLabels"), 
          Comment("data products lables for the PMT fragments from the DAQ"), 
          std::vector<art::InputTag>{ "daq:CAENV1730", "daq:ContainerCAENV1730" }
        };

      };

      using Parameters = art::EDProducer::Table<Config>;

      explicit DaqDecoderSBNDPMT(Parameters const & params);

      DaqDecoderSBNDPMT(DaqDecoderSBNDPMT const &) = delete;
      
      DaqDecoderSBNDPMT(DaqDecoderSBNDPMT &&) = delete;
      
      DaqDecoderSBNDPMT & operator = (DaqDecoderSBNDPMT const &) = delete;
      
      DaqDecoderSBNDPMT & operator = (DaqDecoderSBNDPMT &&) = delete;

      std::vector<art::Handle<artdaq::Fragments>> readHandles( art::Event const & event ) const;

      artdaq::Fragments readFragments( std::vector<art::Handle<artdaq::Fragments>> const& handles ) const;

      void processFragment( const artdaq::Fragment &artdaqFragment );

      void produce(art::Event & e) override;

    private:

      template <std::size_t NBits, typename T>
        static constexpr std::pair<std::array<std::size_t, NBits>, std::size_t>
          setBitIndices(T value) noexcept;
      
      std::vector<art::InputTag> m_input_tags;

      using OpDetWaveformCollection    = std::vector<raw::OpDetWaveform>;
      using OpDetWaveformCollectionPtr = std::unique_ptr<OpDetWaveformCollection>;
      OpDetWaveformCollectionPtr fOpDetWaveformCollection;  
    
      using PMTDigitizerInfoCollection    = std::vector<pmtAnalysis::PMTDigitizerInfo>;
      using PMTDigitizerInfoCollectionPtr = std::unique_ptr<PMTDigitizerInfoCollection>;
      PMTDigitizerInfoCollectionPtr fPMTDigitizerInfoCollection;  

      // Association

  };

}



daq::DaqDecoderSBNDPMT::DaqDecoderSBNDPMT(Parameters const & params)
  : art::EDProducer(params)
  , m_input_tags{ params().FragmentsLabels() }

{

  // Output data products
  produces<std::vector<raw::OpDetWaveform>>();
  produces<std::vector<pmtAnalysis::PMTDigitizerInfo>>();

}


template <std::size_t NBits, typename T>
constexpr std::pair<std::array<std::size_t, NBits>, std::size_t>
daq::DaqDecoderSBNDPMT::setBitIndices(T value) noexcept {
  
  std::pair<std::array<std::size_t, NBits>, std::size_t> res;
  auto& [ indices, nSetBits ] = res;
  for (std::size_t& index: indices) {
    index = (value & 1)? nSetBits++: NBits;
    value >>= 1;
  } // for
  return res;
  
} 


std::vector<art::Handle<artdaq::Fragments>> daq::DaqDecoderSBNDPMT::readHandles( art::Event const & event ) const{

  // Normally or the CAENV1730 or the ContainerCAENV1730 are full
  // We return all the non-empty ones 

  std::vector<art::Handle<artdaq::Fragments>> handles;
  art::InputTag this_input_tag; 

  for( art::InputTag const& input_tag : m_input_tags ) {

    art::Handle<artdaq::Fragments> thisHandle
      = event.getHandle<std::vector<artdaq::Fragment>>(input_tag);
    if( !thisHandle.isValid() || thisHandle->empty() ) continue;

    handles.push_back( thisHandle );

  }

  return handles;

} 


artdaq::Fragments daq::DaqDecoderSBNDPMT::readFragments( std::vector<art::Handle<artdaq::Fragments>> const& handles ) const {

  // Hopefully-not-too-sloppy-code to create the fragment list out of the container  
  artdaq::Fragments fragments;
  
  for( const auto& handle : handles ) {

    assert(!handle->empty());

    if( handle->front().type() == artdaq::Fragment::ContainerFragmentType ) {
      
      for ( auto const& cont : *handle ) {
	
	       artdaq::ContainerFragment contf(cont);
	
	       if( contf.fragment_type() != sbndaq::detail::FragmentType::CAENV1730 ) { break; }
	
	       for( size_t ii=0; ii < contf.block_count(); ii++ ) {
	  
	         fragments.push_back( *(contf[ii]) );
	  
	       }

      } 
      
    }
    
    else {
      
      if ( handle->front().type() != sbndaq::detail::FragmentType::CAENV1730  ) { break; }
	
      for( auto const& frag : *handle ) {
	  
	       fragments.emplace_back( frag );
      }
    } // end if container
  } // end for handles 
  

  return fragments;

}


void daq::DaqDecoderSBNDPMT::processFragment( const artdaq::Fragment &artdaqFragment ) {

  size_t const fragment_id = artdaqFragment.fragmentID();
  size_t const eff_fragment_id = fragment_id & 0x0fff;

  sbndaq::CAENV1730Fragment const fragment(artdaqFragment);
  sbndaq::CAENV1730FragmentMetadata const metafrag = *fragment.Metadata();
  sbndaq::CAENV1730Event const evt = *fragment.Event();
  sbndaq::CAENV1730EventHeader const header = evt.Header;
  size_t const nChannelsPerBoard = metafrag.nChannels;

  
  uint32_t const ev_size_quad_bytes         = header.eventSize;
  uint32_t const evt_header_size_quad_bytes = sizeof(sbndaq::CAENV1730EventHeader)/sizeof(uint32_t);
  uint32_t const data_size_double_bytes     = 2*(ev_size_quad_bytes - evt_header_size_quad_bytes);
  uint32_t const nSamplesPerChannel         = data_size_double_bytes/nChannelsPerBoard;
  uint16_t const enabledChannels      = header.ChannelMask();

  artdaq::Fragment::timestamp_t const fragmentTimestamp = artdaqFragment.timestamp(); 
  unsigned int const time_tag =  header.triggerTimeTag;

  auto const [ chDataMap, nEnabledChannels ] = setBitIndices<16U>(enabledChannels);
  const uint16_t* data_begin = reinterpret_cast<const uint16_t*>(artdaqFragment.dataBeginBytes() + sizeof(sbndaq::CAENV1730EventHeader));

  std::vector<uint16_t> wvfm(nSamplesPerChannel);
  std::vector<float> temperatures;


  for( size_t digitizerChannel=0; digitizerChannel<nChannelsPerBoard; digitizerChannel++ ) {

    std::size_t const pmtID = digitizerChannel+nChannelsPerBoard*eff_fragment_id;

    std::size_t const channelPosInData = chDataMap[digitizerChannel];
    if (channelPosInData >= nEnabledChannels) continue; // not enabled
    std::size_t const ch_offset = channelPosInData * nSamplesPerChannel;

    std::copy_n(data_begin + ch_offset, nSamplesPerChannel, wvfm.begin());

    fOpDetWaveformCollection->emplace_back(fragmentTimestamp, pmtID, wvfm);

    temperatures.push_back( float( metafrag.chTemps[digitizerChannel]) );

  }

  if (nEnabledChannels < 1) temperatures = {-1.0f}; // invalid temperature

  fPMTDigitizerInfoCollection->emplace_back( eff_fragment_id, time_tag, fragmentTimestamp, temperatures );

}


void daq::DaqDecoderSBNDPMT::produce(art::Event & event)
{

  // Make the list of the input fragments 
  try {

    auto fragmentHandles = readHandles( event );

    auto fragments = readFragments( fragmentHandles );

    // initialize the data product 
    fOpDetWaveformCollection = std::make_unique<OpDetWaveformCollection>();
    fPMTDigitizerInfoCollection = std::make_unique<PMTDigitizerInfoCollection>();
    
    if ( !fragments.empty()){
 
      for( auto const & fragment : fragments ) {   
  
          processFragment( fragment );
   
      }
    
    }
    
    else {

       mf::LogError("DaqDecoderSBND") 
	 << "No fragments found\n" << '\n';
    }
    

    // Place the data product in the event stream
    event.put(std::move(fOpDetWaveformCollection));
    event.put(std::move(fPMTDigitizerInfoCollection));

  }

  catch( cet::exception const& e ){

    mf::LogError("DaqDecoderSBND") 
      << "Error while attempting to decode PMT data:\n" << e.what() << '\n';

  }

}

DEFINE_ART_MODULE(daq::DaqDecoderSBNDPMT)

 
