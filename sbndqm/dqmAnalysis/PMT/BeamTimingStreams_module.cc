////////////////////////////////////////////////////////////////////////
// File: BeamTimingStreams_module.cc
// Author: M. Vicenzi (mvicenzi@bnl.gov)
// Date: 02/15/2024
// Description: 
//  Monitor accelerator reference timing signals (RWM and EW) as 
//  digitized in channel 16 of ICARUS PMT digitizers.
// 
////////////////////////////////////////////////////////////////////////

// larsoft framework
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "canvas/Utilities/Exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// data objects
#include "lardataobj/RawData/OpDetWaveform.h"
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"

// metric management
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>

namespace sbndaq {

  class BeamTimingStreams : public art::EDAnalyzer {

    public:

      explicit BeamTimingStreams(fhicl::ParameterSet const & pset);
      template<typename T> T Median(std::vector<T> data) const;
      template<typename T> static size_t getMaxBin(std::vector<T> const& vv, size_t startElement, size_t endElement);
      template<typename T> static size_t getMinBin(std::vector<T> const& vv, size_t startElement, size_t endElement);
      template<typename T> static size_t getStartSample( std::vector<T> const& vv, T thres );
      virtual void analyze(art::Event const & evt);
  
    private:

      // configuration parameters 
      art::InputTag m_opdetwaveform_tag;
      art::InputTag m_trigger_tag;
      std::string m_redis_hostname;
      int m_redis_port;
      fhicl::ParameterSet m_metric_config;
      double m_ADC_threshold;
 
      const unsigned int nChannelsPerBoard = 16;
      const unsigned int nBoards = 24;
      unsigned int nTotalChannels = nBoards*nChannelsPerBoard;
 
  };
}

// ---------------------------------------------------------------------------------------------------------------

sbndaq::BeamTimingStreams::BeamTimingStreams(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_opdetwaveform_tag{ pset.get<art::InputTag>("OpDetWaveformLabel") }
  , m_trigger_tag{ pset.get<art::InputTag>("TriggerLabel") }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "icarus-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
  , m_metric_config{ pset.get<fhicl::ParameterSet>("TimingMetricConfig") }
  , m_ADC_threshold{ pset.get<double>("ADCthreshold", 200)}
{
  // Configure the redis metrics 
  sbndaq::GenerateMetricConfig( m_metric_config );
}

//------------------------------------------------------------------------------------------------------------------

template<typename T> T sbndaq::BeamTimingStreams::Median( std::vector<T> data ) const {

  std::nth_element( data.begin(), data.begin() + data.size()/2, data.end() );
  return data[ data.size()/2 ];

}  

//------------------------------------------------------------------------------------------------------------------

template<typename T> size_t sbndaq::BeamTimingStreams::getMinBin(std::vector<T> const& vv, size_t startElement, size_t endElement ){

  auto minel = std::min_element( vv.begin()+startElement, vv.begin()+endElement );
  size_t minsample = std::distance( vv.begin()+startElement, minel );
  return minsample;

}

//------------------------------------------------------------------------------------------------------------------

template<typename T> size_t sbndaq::BeamTimingStreams::getMaxBin(std::vector<T> const& vv, size_t startElement, size_t endElement){

  auto maxel = std::max_element( vv.begin()+startElement, vv.begin()+endElement );
  size_t maxsample = std::distance( vv.begin()+startElement, maxel );
  return maxsample;

} 

//------------------------------------------------------------------------------------------------------------------

template<typename T> size_t sbndaq::BeamTimingStreams::getStartSample( std::vector<T> const& vv, T thres ){
    
  // get the pulse minimum (undershoot of the signal)
  size_t minbin = getMinBin( vv, 0, vv.size() );

  //search only a cropped region of the waveform backward from the min
  size_t maxbin =  minbin-20; 

  // Now we crawl betweem maxbin and minbin and we stop when:
  // bin value > ( maxbin value - minbin value )*0.2
  size_t startbin = maxbin;
  auto delta = vv[maxbin]-vv[minbin];
 
  if( delta < thres ) // just noise
    return 0; //return first bin 

  for( size_t bin=maxbin; bin<minbin; bin++ ){
    auto val = vv[maxbin]-vv[bin];
    if( val >= 0.2*delta ){  // 20%
      startbin = bin - 1;
      break;
    }
  }

  if( startbin < maxbin ){
    startbin=maxbin;
  }

  return startbin;
}


//------------------------------------------------------------------------------------------------------------------
void sbndaq::BeamTimingStreams::analyze(art::Event const & evt) {

  //int metric_level = 3; 
  //artdaq::MetricMode mode = artdaq::MetricMode::Average;
  //artdaq::MetricMode last = artdaq::MetricMode::LastPoint;
  //std::string groupName = "beam_timing";

  // find RWM and EW times from the waveforms
  art::Handle opdetHandle = evt.getHandle<std::vector<raw::OpDetWaveform>>( m_opdetwaveform_tag);
  if( opdetHandle.isValid() && !opdetHandle->empty() ) {
    
    for ( auto const & opdetwaveform : *opdetHandle ) {
    	auto id = opdetwaveform.ChannelNumber();
        //auto ts = opdetwaveform.TimeStamp();	
	auto len = opdetwaveform.Waveform().size();
	std::cout << "event " << evt.id().event() << " om pmt channel " << id << " -> " << len << std::endl;
   }  
  }else{
    mf::LogError("sbndaq::BeamTimingStreams::analyze") << "Data product '" << m_opdetwaveform_tag.encode() << "' has no raw::OpDetWaveform in it!\n";
  }

/*
    // Create a sample with only one waveforms per channel

    std::vector<unsigned int> m_unique_channels; 

    for ( auto const & opdetwaveform : *opdetHandle ) {

      if( m_unique_channels.size() > nTotalChannels ) { break; }

      unsigned int const pmtId = opdetwaveform.ChannelNumber();
      
      auto findIt = std::find( m_unique_channels.begin(), m_unique_channels.end(), pmtId );

      if( findIt != m_unique_channels.end() ) { continue; }
		
      // We have never seen this channel before
      m_unique_channels.push_back( pmtId );
      
      std::string pmtId_s = std::to_string(pmtId);

      int16_t baseline = Median( opdetwaveform );
	
      pulseRecoManager.Reconstruct( opdetwaveform );
      std::vector<double>  pedestal_sigma = pedAlg->Sigma();
        
      double rms = Median( pedestal_sigma );

      auto const& pulses = threshAlg->GetPulses();
      double npulses = (double)pulses.size();

      //std::cout << pmtId << " " << baseline << " " << rms << " " << npulses << std::endl;

      // Send the metrics 
      sbndaq::sendMetric(groupName, pmtId_s, "baseline", baseline, level, mode); // Send baseline information
      sbndaq::sendMetric(groupName, pmtId_s, "rms", rms, level, mode); // Send rms information
      sbndaq::sendMetric(groupName, pmtId_s, "rate", npulses, level, rate); // Send rms information
        

      // Now we send a copy of the waveforms 
      double tickPeriod = 0.002; // [us] 
      std::vector<std::vector<raw::ADC_Count_t>> adcs {opdetwaveform};
      std::vector<int> start { 0 }; // We are considreing each waveform independent for now 

      sbndaq::SendSplitWaveform("snapshot:waveform:PMT:" + pmtId_s, adcs, start, tickPeriod);
      sbndaq::SendEventMeta("snapshot:waveform:PMT:" + pmtId_s, evt);

    } // for      

    if( m_unique_channels.size() < nTotalChannels ) {

         mf::LogError("sbndaq::CAENV1730Streams::analyze") 
          << "Event has " << m_unique_channels.size() << " channels, " << nTotalChannels << " expected.\n";
    }

  }

  else {

     mf::LogError("sbndaq::CAENV1730Streams::analyze") 
          << "Data product '" << m_opdetwaveform_tag.encode() << "' has no raw::OpDetWaveform in it!\n";

  }
 */
}

DEFINE_ART_MODULE(sbndaq::BeamTimingStreams)
