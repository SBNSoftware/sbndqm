////////////////////////////////////////////////////////////////////////
// Class:       TriggerStreams
// Plugin Type: analyzer
// File:        TriggerStreams_module.cc
// Author:      A. Scarpelli, reworked by M. Vicenzi (mvicenzi@bnl.gov)
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "canvas/Utilities/Exception.h"

#include "sbndqm/Decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include "lardataobj/RawData/ExternalTrigger.h"
#include "lardataobj/RawData/TriggerData.h"
#include "lardataobj/Simulation/BeamGateInfo.h"
#include "sbndqm/Decode/Trigger/detail/TriggerGateTypes.h"

#include <cassert>
#include <stdio.h>
#include <iomanip>
#include <vector>
#include <iostream>
#include <string>

#include "messagefacility/MessageLogger/MessageLogger.h"


namespace sbndaq {

  class TriggerStreams : public art::EDAnalyzer {

    public:

        explicit TriggerStreams(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization 
        
        std::string bitsToName(int source);

        virtual void analyze(art::Event const & evt);
  
    private:
    
        art::InputTag m_trigger_tag;

        std::string m_redis_hostname;
        int m_redis_port;
        
        fhicl::ParameterSet m_metric_config;

        void clean();
   
        std::unordered_map<int, int> bitsCountsMap;
  };

}

//------------------------------------------------------------------------------------------------------------------

sbndaq::TriggerStreams::TriggerStreams(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_trigger_tag{ pset.get<art::InputTag>("TriggerLabel") }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "icarus-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
  , m_metric_config{ pset.get<fhicl::ParameterSet>("TriggerMetricConfig") }
{

  // Configure the redis metrics 
  sbndaq::GenerateMetricConfig( m_metric_config );
}

//------------------------------------------------------------------------------------------------------------------

void sbndaq::TriggerStreams::clean() {
  
  bitsCountsMap.clear();

}

//------------------------------------------------------------------------------------------------------------------

std::string sbndaq::TriggerStreams::bitsToName(int source){

  switch(source){
    case daq::TriggerGateTypes::BNB:
      return "BNB";    
    case daq::TriggerGateTypes::OffbeamBNB:
      return "OffbeamBNB";    
    case daq::TriggerGateTypes::NuMI:
      return "NuMI";    
    case daq::TriggerGateTypes::OffbeamNuMI:
      return "OffbeamNuMI";    
    case daq::TriggerGateTypes::Calib:
      return "Unknown";    
    default:
      return "Unknown";
  }

}

//------------------------------------------------------------------------------------------------------------------

void sbndaq::TriggerStreams::analyze(art::Event const & evt) {

  //Redis stuff
  int level = 3; 
  std::string groupName = "TriggerRates";
  artdaq::MetricMode rate = artdaq::MetricMode::Rate;

  // Now we get the trigger information
  art::Handle triggerHandle = evt.getHandle<std::vector<raw::Trigger>>(m_trigger_tag);

  if( triggerHandle.isValid() && !triggerHandle->empty() ) {
    for( auto const trigger : *triggerHandle ){
      bitsCountsMap[ trigger.TriggerBits() ]++;
    }
  }   
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product raw::Trigger not found!\n";
  }

  // Here we read the external trigger information
  art::Handle extTriggerHandle = evt.getHandle<std::vector<raw::ExternalTrigger>>(m_trigger_tag);

  if( extTriggerHandle.isValid() && !extTriggerHandle->empty() ) {
    //std::cout << "OK" << std::endl;
  }
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product raw::ExternalTrigger not found!\n";
  }
  
  // Here we read the beam information
  art::Handle gateHandle = evt.getHandle<std::vector<sim::BeamGateInfo>>(m_trigger_tag);

  if( gateHandle.isValid() && !gateHandle->empty() ) {
    //std::cout << "OK" << std::endl;
  }
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product sim::GeteBeamInfo not found!\n";
  }

  // Now we send the metrics. There is probably one trigger per event
  // but agnostically we created a map to count the different types, hence we send them separately
  for( const auto bits : bitsCountsMap ){
    
    std::string triggerid_s = std::to_string(bits.first);
     
    //Send the trigger rate 
    sbndaq::sendMetric(groupName, triggerid_s, "trigger_rate", bits.second, level, rate);
    
  }
  
  // Sweep the dust away 
  clean();

}

DEFINE_ART_MODULE(sbndaq::TriggerStreams)
