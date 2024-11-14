////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderIcarusPMT
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderIcarusPMT_module.cc
// Author:      A. Scarpelli, with great help of G. Petrillo et al.
//		Partially reworked by M. Vicenzi (mvicenzi@bnl.gov)
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

  // support structure for the decoding
  // storing  raw::OpDetWaveforms AND the TTT
  struct ProtoWaveform 
  {
    raw::OpDetWaveform wf;
    unsigned int TTT;
    ProtoWaveform(const raw::OpDetWaveform waveform, unsigned int ttt):
      wf(waveform), TTT(ttt) {};
  };


  class DaqDecoderIcarusPMT : public art::EDProducer 
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

      explicit DaqDecoderIcarusPMT(Parameters const & params);
      DaqDecoderIcarusPMT(DaqDecoderIcarusPMT const &) = delete;
      DaqDecoderIcarusPMT(DaqDecoderIcarusPMT &&) = delete; 
      DaqDecoderIcarusPMT & operator = (DaqDecoderIcarusPMT const &) = delete;
      DaqDecoderIcarusPMT & operator = (DaqDecoderIcarusPMT &&) = delete;

      // read & process fragments
      std::vector<art::Handle<artdaq::Fragments>> readHandles( art::Event const & event ) const;
      artdaq::Fragments readFragments( std::vector<art::Handle<artdaq::Fragments>> handles );
      void processFragment( const artdaq::Fragment &artdaqFragment );

      // stitch contiguous waveforms
      void stitchWaveforms();
      void sortWaveformsByTime(std::vector<ProtoWaveform> &wfs) const;
      raw::OpDetWaveform mergeWaveformGroup(std::vector<ProtoWaveform> &all_wfs, std::vector<int> const& group);

      void produce(art::Event & e) override;

    private:

      template <std::size_t NBits, typename T> static constexpr 
        std::pair<std::array<std::size_t, NBits>, std::size_t> setBitIndices(T value) noexcept;
      template <typename T> std::vector<T>& appendTo( std::vector<T>& dest, std::vector<T>&& src);
      
      std::vector<art::InputTag> fFragmentLabels;
      std::map<std::size_t,std::vector<ProtoWaveform>> fProtoWaveforms;
      double fOpticalTickNS;

      using OpDetWaveformCollection    = std::vector<raw::OpDetWaveform>;
      using OpDetWaveformCollectionPtr = std::unique_ptr<OpDetWaveformCollection>;
      OpDetWaveformCollectionPtr fOpDetWaveformCollection;  
    
      using PMTDigitizerInfoCollection    = std::vector<pmtAnalysis::PMTDigitizerInfo>;
      using PMTDigitizerInfoCollectionPtr = std::unique_ptr<PMTDigitizerInfoCollection>;
      PMTDigitizerInfoCollectionPtr fPMTDigitizerInfoCollection;  

  };

}

// -----------------------------------------------------------------------------------------

daq::DaqDecoderIcarusPMT::DaqDecoderIcarusPMT(Parameters const & params)
  : art::EDProducer(params)
  , fFragmentLabels{ params().FragmentsLabels() }
  , fOpticalTickNS{ 2.0 }
{
  
  // Output data products
  produces<std::vector<raw::OpDetWaveform>>();
  produces<std::vector<pmtAnalysis::PMTDigitizerInfo>>();

}

// -----------------------------------------------------------------------------------------

template <std::size_t NBits, typename T> constexpr std::pair<std::array<std::size_t, NBits>, std::size_t> 
daq::DaqDecoderIcarusPMT::setBitIndices(T value) noexcept {
  
  std::pair<std::array<std::size_t, NBits>, std::size_t> res;
  auto& [ indices, nSetBits ] = res;
  for (std::size_t& index: indices) {
    index = (value & 1)? nSetBits++: NBits;
    value >>= 1;
  } // for
  return res;
  
} 

// -----------------------------------------------------------------------------------------

std::vector<art::Handle<artdaq::Fragments>> daq::DaqDecoderIcarusPMT::readHandles( art::Event const & event ) const{

  // Normally or the CAENV1730 or the ContainerCAENV1730 are full
  // We return all the non-empty ones 

  std::vector<art::Handle<artdaq::Fragments>> handles;
  art::InputTag this_input_tag; 

  for( art::InputTag const& input_tag : fFragmentLabels ) {

    art::Handle<artdaq::Fragments> thisHandle = 
      event.getHandle<std::vector<artdaq::Fragment>>(input_tag);
    if( !thisHandle.isValid() || thisHandle->empty() ) continue;

    handles.push_back( thisHandle );

  }

  return handles;
} 

// -----------------------------------------------------------------------------------------

artdaq::Fragments daq::DaqDecoderIcarusPMT::readFragments( std::vector<art::Handle<artdaq::Fragments>> handles ) {

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
      if( handle->front().type() != sbndaq::detail::FragmentType::CAENV1730  ) { break; }
      for( auto frag : *handle ) {
        fragments.emplace_back( frag );
      }
    } // end if container
  } // end for handles 
 
  return fragments;

}


// -----------------------------------------------------------------------------------------

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

  // The fragment timestamp is computed in the DAQ using the server time and the TTT
  // It is pointing at the END of the saved buffer, and in ns
  artdaq::Fragment::timestamp_t const fragmentTimestamp = artdaqFragment.timestamp(); 
  unsigned int const time_tag =  header.triggerTimeTag;

  auto const [ chDataMap, nEnabledChannels ] = setBitIndices<16U>(enabledChannels);
  const uint16_t* data_begin = reinterpret_cast<const uint16_t*>(artdaqFragment.dataBeginBytes() + sizeof(sbndaq::CAENV1730EventHeader));

  // Channel waveform + channel temperatures
  std::vector<uint16_t> wvfm(nSamplesPerChannel);
  std::vector<float> temperatures;

  for( size_t digitizerChannel=0; digitizerChannel<nChannelsPerBoard; digitizerChannel++ ) {

    // We don't have access to the offline DB: this is a different channel mapping
    // Also offline DB doesn't deal with special channels (16th) 
    std::size_t const pmtID = digitizerChannel+nChannelsPerBoard*eff_fragment_id;

    std::size_t const channelPosInData = chDataMap[digitizerChannel];
    if (channelPosInData >= nEnabledChannels) continue; // not enabled
    std::size_t const ch_offset = channelPosInData * nSamplesPerChannel;

    // get the waveform
    std::copy_n(data_begin + ch_offset, nSamplesPerChannel, wvfm.begin());

    // saves all waveforms from this channel in the event + TTT for time stitching
    auto this_wf =  raw::OpDetWaveform(fragmentTimestamp, pmtID, wvfm);
    fProtoWaveforms[pmtID].emplace_back(this_wf, time_tag);

    temperatures.push_back( float( metafrag.chTemps[digitizerChannel] ));

  }

  fPMTDigitizerInfoCollection->emplace_back( eff_fragment_id, time_tag, fragmentTimestamp, temperatures );

}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusPMT::sortWaveformsByTime(std::vector<ProtoWaveform> &wfs) const {
  
  auto time_sorting = [](ProtoWaveform const& left, ProtoWaveform const& right){
    return left.wf.TimeStamp() < right.wf.TimeStamp();
  };

  std::sort( wfs.begin(), wfs.end(), time_sorting);
}

// -----------------------------------------------------------------------------------------

// Moves the contend of `src` into the end of `dest`.
template <typename T> std::vector<T>& daq::DaqDecoderIcarusPMT::appendTo(std::vector<T>& dest, std::vector<T>&& src) {

  if (dest.empty()) dest = std::move(src);
  else {
    dest.reserve(dest.size() + src.size());
    std::move(src.begin(), src.end(), std::back_inserter(dest));
  }
  
  src.clear();
  return dest;
} 

// -----------------------------------------------------------------------------------------

raw::OpDetWaveform daq::DaqDecoderIcarusPMT::mergeWaveformGroup(std::vector<ProtoWaveform> &all_wfs, std::vector<int> const& group){

  auto it = group.begin();
  auto itEnd = group.end();

  // put inside the first waveform of the group
  raw::OpDetWaveform  mergedWaveform{ std::move(all_wfs.at(*it).wf) }; 

  mf::LogTrace("DaqDecoderIcarusPMT") << "NEW merged group for " << mergedWaveform.ChannelNumber() << "\n" 
  				      << "wf timestamp " << mergedWaveform.TimeStamp() 
                                      << " TTT " << all_wfs.at(*it).TTT 
				      << " size " << mergedWaveform.Waveform().size()
      				      << " TTT*8ns-size*2ns " << all_wfs.at(*it).TTT*8 - mergedWaveform.Waveform().size()*2;
  unsigned int last_end = all_wfs.at(*it).TTT*8;

  // append all the others (if any)
  while( ++it != itEnd ){
    
    raw::OpDetWaveform &wf = all_wfs.at(*it).wf;
    unsigned int expected_size = mergedWaveform.Waveform().size() + wf.Waveform().size();
  
    mf::LogTrace("DaqDecoderIcarusPMT") << "wf timestamp " << wf.TimeStamp() 
					<< " TTT " << all_wfs.at(*it).TTT 
					<< " size " << wf.Waveform().size()
    					<< " TTT*8ns-size*2ns " << all_wfs.at(*it).TTT*8 - wf.Waveform().size()*2
    					<< " TTT*8ns-size*2ns - last_end " << all_wfs.at(*it).TTT*8 - wf.Waveform().size()*2 - last_end << std::endl;
    last_end = all_wfs.at(*it).TTT*8;
    
    appendTo( mergedWaveform.Waveform() , std::move(wf.Waveform()));

    if( mergedWaveform.Waveform().size() != expected_size )
	 mf::LogError("DaqDecoderIcarusPMT") << "Error in waveform size after merge!";
  }

  mf::LogTrace("DaqDecoderIcarusPMT") << "SIZE " << mergedWaveform.Waveform().size();

  return mergedWaveform;
}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusPMT::stitchWaveforms(){

  // One event can produce several fragments, i.e. multiple waveforms per channel
  // some of these waveforms are contigous (due to trigger overlap), especially around the global trigger
  // contigous waveforms must be stiched together into a single waveform!

  // for each channel
  for(auto it=fProtoWaveforms.begin(); it!=fProtoWaveforms.end(); it++){
  
    std::vector<ProtoWaveform> this_wfs = it->second;
    int nwfs = it->second.size();
    if( nwfs < 2.) continue;  //nothing to stitch

    sortWaveformsByTime(this_wfs); //sort by time

    // group together the indices of waveforms that should be merged together
    std::vector< std::vector<int> > groups;
    int iWave = 0; 
    do {
      
      // place next waveform in a new group
      std::vector<int> current_group { iWave };
      double current_end = this_wfs[iWave].TTT;      

      // scan waveforms that come next
      while(++iWave < nwfs){
        
        // if too apart, skip and start a new group
        // note TTT counts every 8 ns!
        double next_start_time = this_wfs[iWave].TTT*8 - this_wfs[iWave].wf.Waveform().size()*fOpticalTickNS; 
        if( next_start_time - current_end*8 > 16 ) break;

        // the next one is contiguos: assign it to the same group and look for the next one 
	current_group.push_back(iWave);
	current_end = this_wfs[iWave].TTT;
      }
      
      groups.push_back(current_group);  
    } while ( iWave < nwfs );

    // Now, for each group, we merge them and add to the final collection
    for(auto const& group: groups){
      auto mergedWaveform = mergeWaveformGroup( this_wfs, group);
      fOpDetWaveformCollection->push_back(std::move(mergedWaveform));
    }
  }
}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusPMT::produce(art::Event & event)
{

  mf::LogInfo("DaqDecoderIcarusPMT") << "Decoding PMT fragments...";

  // clear proto-waveforms/TTTs from previous event      
  fProtoWaveforms.clear();
  
  // Make the list of the input fragments 
  try {

    auto fragmentHandles = readHandles( event );
    auto fragments = readFragments( fragmentHandles );

    // initialize the data product 
    fOpDetWaveformCollection = std::make_unique<OpDetWaveformCollection>();
    fPMTDigitizerInfoCollection = std::make_unique<PMTDigitizerInfoCollection>();
    
    if ( !fragments.empty()){

      for( auto const & fragment : fragments ) {   
        processFragment( fragment ); // reads data
        stitchWaveforms(); // merge together contiguos waveforms
      } 
    }
    
    else {
       mf::LogError("DaqDecoderIcarusPMT") 
	 << "No fragments found\n" << '\n';
    }
    
    // Place the data product in the event stream
    event.put(std::move(fOpDetWaveformCollection));
    event.put(std::move(fPMTDigitizerInfoCollection));

  }

  catch( cet::exception const& e ){
    mf::LogError("DaqDecoderIcarus") 
      << "Error while attempting to decode PMT data:\n" << e.what() << '\n';
    
    // fill empty products
    fOpDetWaveformCollection = std::make_unique<OpDetWaveformCollection>();
    fPMTDigitizerInfoCollection = std::make_unique<PMTDigitizerInfoCollection>();
    event.put(std::move(fOpDetWaveformCollection));
    event.put(std::move(fPMTDigitizerInfoCollection));
  }
  
  mf::LogInfo("DaqDecoderIcarusPMT") << "PMT fragments decoded!";

}

DEFINE_ART_MODULE(daq::DaqDecoderIcarusPMT)
