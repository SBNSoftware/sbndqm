////////////////////////////////////////////////////////////////////////
// 
// TriggerStreams_module.cc 
// 
// Andrea Scarpelli ( ascarpell@bnl.gov )
//
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "canvas/Utilities/Exception.h"

#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"
#include "sbndqm/Decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include "lardataobj/RawData/ExternalTrigger.h"
#include "lardataobj/RawData/TriggerData.h"
#include "lardataobj/Simulation/BeamGateInfo.h"

#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>


#include "messagefacility/MessageLogger/MessageLogger.h"


namespace sbndaq {

  class TriggerStreams : public art::EDAnalyzer {

    public:

        explicit TriggerStreams(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  
        virtual void analyze(art::Event const & evt);
  
    private:
    
        art::InputTag m_trigger_tag;

        std::string m_redis_hostname;
        int m_redis_port;
        double stringTime = 0.0;

        fhicl::ParameterSet m_metric_config;

        void clean();
    
  };

}

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


void sbndaq::TriggerStreams::clean() {}


//------------------------------------------------------------------------------------------------------------------


void sbndaq::TriggerStreams::analyze(art::Event const & evt) {

  // Now we get the trigger information
  art::Handle< std::vector<raw::Trigger> > triggerHandle;
  evt.getByLabel( m_trigger_tag, triggerHandle );

  if( triggerHandle.isValid() && !triggerHandle->empty() ) {
    std::cout << "OK" << std::endl;
  }   
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product raw::Trigger not found!\n";
  }

  // Here we read the external trigger information
  art::Handle< std::vector<raw::ExternalTrigger> > extTriggerHandle;
  evt.getByLabel( m_trigger_tag, extTriggerHandle );

  if( extTriggerHandle.isValid() && !extTriggerHandle->empty() ) {
    std::cout << "OK" << std::endl;
  }
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product raw::ExternalTrigger not found!\n";
  }
  
  // Here we read the beam information
  art::Handle< std::vector<sim::BeamGateInfo> > gateHandle;
  evt.getByLabel( m_trigger_tag, gateHandle );

  if( gateHandle.isValid() && !gateHandle->empty() ) {
    std::cout << "OK" << std::endl;
  }
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product sim::GeteBeamInfo not found!\n";
  }


 

}

DEFINE_ART_MODULE(sbndaq::TriggerStreams)
