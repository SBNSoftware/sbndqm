////////////////////////////////////////////////////////////////////////
// File    : BeamTimingStreams_module.cc
// Author  : M. Vicenzi (mvicenzi@bnl.gov)
// Date    : 02/15/2024
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
#include "sbndqm/Decode/Trigger/detail/TriggerGateTypes.h"
#include "lardataobj/RawData/ExternalTrigger.h"
#include "lardataobj/RawData/TriggerData.h"

// metric management
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include <algorithm>
#include <cassert>
#include <vector>
#include <iostream>
#include <string>

namespace sbndaq {

  class BeamTimingStreams : public art::EDAnalyzer {

    public:

      explicit BeamTimingStreams(fhicl::ParameterSet const & pset);
      
      template<typename T> T Median(std::vector<T> data) const;
      template<typename T> static size_t getMaxBin(std::vector<T> const& vv, size_t startElement, size_t endElement);
      template<typename T> static size_t getMinBin(std::vector<T> const& vv, size_t startElement, size_t endElement);
      template<typename T> static size_t getStartSample( std::vector<T> const& vv, T thres );
      
      std::string getGateName(int source);
      bool isSpecialChannel (int channel, std::map<size_t,std::string> channel_map, int &crateID);

      virtual void analyze(art::Event const & evt);
  
    private:

      // configuration parameters 
      art::InputTag m_opdetwaveform_tag;
      art::InputTag m_trigger_tag;
      std::string m_redis_hostname;
      int m_redis_port;
      fhicl::ParameterSet m_metric_config;
      short int m_ADC_threshold;
      double m_OpticalTick; 

      const unsigned int nChannelsPerBoard = 16;
      const unsigned int nBoards = 24;
      unsigned int nTotalChannels = nBoards*nChannelsPerBoard;
  
      std::map<size_t, std::string>  RWMboards {
	{ 0x2017, "eebot03" }, 
	{ 0x2014, "eetop03" }, 
	{ 0x2011, "ewbot03" }, 
	{ 0x200E, "ewtop03" }, 
	{ 0x200B, "webot03" }, 
	{ 0x2008, "wetop03" }, 
	{ 0x2005, "wwbot03" }, 
	{ 0x2002, "wwtop03" } 
      };
      std::map<size_t, std::string>  EWboards {
	{ 0x2016, "eebot02" }, 
	{ 0x2013, "eetop02" }, 
	{ 0x2010, "ewbot02" },
	{ 0x200D, "ewtop02" }, 
	{ 0x200A, "webot02" }, 
	{ 0x2007, "wetop02" }, 
	{ 0x2004, "wwbot02" }, 
	{ 0x2001, "wwtop02" }, 
      };
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
  , m_ADC_threshold{ pset.get<short int>("ADCthreshold", 200)}
  , m_OpticalTick{ 0.002 }
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

std::string sbndaq::BeamTimingStreams::getGateName(int source){
  
  switch(source){
    case daq::TriggerGateTypes::BNB:
      return "BNB";
    case daq::TriggerGateTypes::NuMI:
      return "NUMI";
    default:
      return "Offbeam";
  }
  
}

//------------------------------------------------------------------------------------------------------------------

bool sbndaq::BeamTimingStreams::isSpecialChannel(int pmtID, std::map<size_t,std::string> channel_map, int &crateID){

  int digitizer_last_channel = 15;
  crateID = -1;
  for(auto it=channel_map.begin(); it!=channel_map.end(); it++){ 
    size_t eff_fragment_id = it->first & 0x0fff;
    int spare_channel = eff_fragment_id*nChannelsPerBoard + digitizer_last_channel;
    if( spare_channel == pmtID ){
      crateID = int(eff_fragment_id / 3);
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------------------------------------------

void sbndaq::BeamTimingStreams::analyze(art::Event const & evt) {

  mf::LogInfo("BeamTimingStreams") << "Computing BeamTiming metrics...";

  int level = 3; 
  //artdaq::MetricMode mode = artdaq::MetricMode::Average;
  artdaq::MetricMode mode = artdaq::MetricMode::LastPoint;
  std::string groupName = "BeamTiming";

  // determine gate type, if offbeam skip event!
  art::Handle triggerHandle = evt.getHandle<std::vector<raw::Trigger>>(m_trigger_tag);
  
  if( !triggerHandle.isValid() || triggerHandle->empty() ) {
    mf::LogError("sbndaq::BeamTimingStreams::analyze") 
      << "Data product '" << m_trigger_tag.encode() << "' has no raw::Trigger in it!";
    return;
  }
  
  if( (*triggerHandle).size()>1 )
    mf::LogWarning("sbndaq::BeamTimingStreams::analyze") << "More than one trigger in the event!";

  auto trigger_source = (*triggerHandle).at(0).TriggerBits();
  std::string metric_prefix = getGateName(trigger_source);

  if( metric_prefix != "BNB" && metric_prefix != "NUMI" ){
    mf::LogInfo("BeamTimingStreams") << "Skipping because trigger is " << metric_prefix << "...";  
    return;
  } 

  // now you need to select the correct waveforms:
  // - must be the channel id corresponding to a special channel (RWM/EW)
  // - must be the on-beam waveform (longest one)
  art::Handle opdetHandle = evt.getHandle<std::vector<raw::OpDetWaveform>>( m_opdetwaveform_tag);

  if( !opdetHandle.isValid() || opdetHandle->empty() ) {
    mf::LogError("sbndaq::BeamTimingStreams::analyze") 
      << "Data product '" << m_opdetwaveform_tag.encode() << "' has no raw::OpDetWaveform in it!";
    return;
  }
    
  std::map<int, unsigned int> rwm_wfs; 
  std::map<int, unsigned int> ew_wfs; 
  unsigned int index = 0;

  for ( auto const & opdetwaveform : *opdetHandle ) {

    unsigned int const pmtId = opdetwaveform.ChannelNumber();
    std::size_t length = opdetwaveform.Waveform().size();
    int crate = -1;

    // RWM
    if( isSpecialChannel(pmtId, RWMboards, crate) ){

      if( rwm_wfs.find(crate) == rwm_wfs.end() ){
        rwm_wfs[crate] = index;
        index++;
        continue;
      }
      unsigned int seen_index = rwm_wfs[crate];
      if( (*opdetHandle)[seen_index].Waveform().size() < length )
        rwm_wfs[crate] = index;
    } 
    
    // EW
    else if( isSpecialChannel(pmtId, EWboards, crate) ){

      if( ew_wfs.find(crate) == ew_wfs.end() ){
        ew_wfs.insert(std::make_pair(crate,index));
        index++;
        continue;
      }
      unsigned int seen_index = ew_wfs[crate];
      if( (*opdetHandle)[seen_index].Waveform().size() < length )
        ew_wfs[crate] = index;
    } 

    index++;
  }
  
  if( rwm_wfs.size() < 8 || ew_wfs.size() < 8 ) {
    mf::LogError("sbndaq::BeamTimingStreams::analyze") 
      << "Event " << evt.id().event() << " is missing special waveforms: rwm: " << rwm_wfs.size() << ", ew: " << ew_wfs.size();
  }

  // For each crate, extract the EW and RWM time form the waveforms
  // Compute the RWM-EW difference and send it as metric
  // Send a copy of the EW/RW, waveforms as well for each crate

  for( auto it=rwm_wfs.begin(); it != rwm_wfs.end(); it++ ){
    
    double RWM_start = getStartSample( (*opdetHandle)[it->second].Waveform(), m_ADC_threshold );
    double EW_start = getStartSample( (*opdetHandle)[ew_wfs[it->first]].Waveform(), m_ADC_threshold );
    double RWM_EW = (RWM_start-EW_start)*m_OpticalTick; //us

    // sometimes an extremely negative difference is found: -3.6 x 10^6
    // this is clearly unphysical, so catch these occurances
    // since they mess up the plot autoscale, replace with 0 (=missing signals)
    if( RWM_EW < -100. || RWM_EW > 100. ){
      mf::LogError("sbndaq::BeamTimingStreams::analyze") 
        << "Event " << evt.id().event() << ", Crate " << it->first << " with unphysical " << metric_prefix << " RWM-EW :" << RWM_EW << " us!"
        << " EW: wf_index " << ew_wfs[it->first] << " wf_size " << (*opdetHandle)[ew_wfs[it->first]].Waveform().size() << " ew_start " << EW_start
        << " RWM: wf_index " << it->second << " wf_size " << (*opdetHandle)[it->second].Waveform().size() << " rwm_start " << RWM_start;
      RWM_EW = 0.;
    }

    // send RWM-EW metric
    std::string instance_s = std::to_string(it->first); // this it the crate number
    std::string metric_name = metric_prefix + "_RWM_EW"; // either BNB or NuMI
    sbndaq::sendMetric(groupName, instance_s, metric_name, RWM_EW, level, mode);
   
    // send RWM waveform
    std::vector<raw::ADC_Count_t> RWMadcs { (*opdetHandle)[it->second].Waveform() };
    std::string rwm_wf_instance = "snapshot:" + metric_prefix + "_RWM:" + instance_s;
    sbndaq::SendWaveform(rwm_wf_instance, RWMadcs, m_OpticalTick);
    sbndaq::SendEventMeta(rwm_wf_instance, evt);

    // send EW waveform    
    std::vector<raw::ADC_Count_t> EWadcs { (*opdetHandle)[ew_wfs[it->first]].Waveform() };
    std::string ew_wf_instance = "snapshot:" + metric_prefix + "_EW:" + instance_s;
    sbndaq::SendWaveform(ew_wf_instance, EWadcs, m_OpticalTick);
    sbndaq::SendEventMeta(ew_wf_instance, evt);
  }

  mf::LogInfo("BeamTimingStreams") << "BeamTiming metrics sent!";

}

DEFINE_ART_MODULE(sbndaq::BeamTimingStreams)
