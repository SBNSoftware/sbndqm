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

      const unsigned int nChannelsPerBoard = 16;
      const unsigned int nBoards = 24;
      unsigned int nTotalChannels = nBoards*nChannelsPerBoard;
        
        art::InputTag m_opdetwaveform_tag;

        std::string m_redis_hostname;
        
        int m_redis_port;

        double stringTime = 0.0;

        fhicl::ParameterSet m_metric_config;

        pmtana::PulseRecoManager pulseRecoManager;
        pmtana::PMTPulseRecoBase* threshAlg;
        pmtana::PMTPedestalBase*  pedAlg;

 
        void clean();

        template<typename T> T Median( std::vector<T> data ) const ;

        //int16_t Median(std::vector<int16_t> data, size_t n_adc);
        
        //double Median(std::vector<double> data, size_t n_adc);
      
        //double RMS(std::vector<int16_t> const& data, size_t n_adc, int16_t baseline ) const;
    
  };


}





sbndaq::TriggerStreams::TriggerStreams(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_opdetwaveform_tag{ pset.get<art::InputTag>("OpDetWaveformLabel") }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "icarus-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
{

  // Configure the redis metrics 
  sbndaq::GenerateMetricConfig( m_metric_config );
}

//------------------------------------------------------------------------------------------------------------------


void sbndaq::TriggerStreams::clean() {

}


//------------------------------------------------------------------------------------------------------------------


void sbndaq::TriggerStreams::analyze(art::Event const & evt) {

  std::cout << "Printing something" << std::endl;

  //int level = 3; 

  //artdaq::MetricMode mode = artdaq::MetricMode::Average;
  //artdaq::MetricMode rate = artdaq::MetricMode::Rate;

  //std::string groupName = "Trigger";


  // Now we look at the waveforms 
  //art::Handle< std::vector<raw::OpDetWaveform> > opdetHandle;
  //evt.getByLabel( m_opdetwaveform_tag, opdetHandle );

  //if( opdetHandle.isValid() && !opdetHandle->empty() ) {

      // Send the metrics 
      //sbndaq::sendMetric(groupName, pmtId_s, "baseline", baseline, level, mode); // Send baseline information
      //sbndaq::sendMetric(groupName, pmtId_s, "rms", rms, level, mode); // Send rms information
      //sbndaq::sendMetric(groupName, pmtId_s, "rate", npulses, level, rate); // Send rms information
        

      // Now we send a copy of the waveforms 
      //double tickPeriod = 0.002; // [us] 
      //std::vector<std::vector<raw::ADC_Count_t>> adcs {opdetwaveform};
      //std::vector<int> start { 0 }; // We are considreing each waveform independent for now 

      //sbndaq::SendSplitWaveform("snapshot:waveform:PMT:" + pmtId_s, adcs, start, tickPeriod);
      //sbndaq::SendEventMeta("snapshot:waveform:PMT:" + pmtId_s, evt);

  //}

//else {

    //mf::LogError("sbndaq::CAENV1730Streams::analyze") 
       //   << "Data product '" << m_opdetwaveform_tag.encode() << "' has no raw::OpDetWaveform in it!\n";

     // }
 

  // In case we need to clean something
  //clean();

  // Ronf for two seconds 
  //sleep(2); // Is it still necessary ? uncomment if so
   
}

DEFINE_ART_MODULE(sbndaq::TriggerStreams)
