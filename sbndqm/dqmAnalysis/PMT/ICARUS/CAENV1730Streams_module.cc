////////////////////////////////////////////////////////////////////////
//
// 
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "canvas/Utilities/Exception.h"

#include "lardataobj/RawData/OpDetWaveform.h"
#include "larana/OpticalDetector/OpHitFinder/PMTPulseRecoBase.h"
#include "larana/OpticalDetector/OpHitFinder/AlgoThreshold.h"
#include "larana/OpticalDetector/OpHitFinder/AlgoSiPM.h"
#include "larana/OpticalDetector/OpHitFinder/AlgoSlidingWindow.h"
#include "larana/OpticalDetector/OpHitFinder/AlgoFixedWindow.h"
#include "larana/OpticalDetector/OpHitFinder/AlgoCFD.h"
#include "larana/OpticalDetector/OpHitFinder/PedAlgoEdges.h"
#include "larana/OpticalDetector/OpHitFinder/PedAlgoRollingMean.h"
#include "larana/OpticalDetector/OpHitFinder/PedAlgoUB.h"
#include "larana/OpticalDetector/OpHitFinder/PulseRecoManager.h"

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

  class CAENV1730Streams : public art::EDAnalyzer {

    public:

        explicit CAENV1730Streams(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  
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





sbndaq::CAENV1730Streams::CAENV1730Streams(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_opdetwaveform_tag{ pset.get<art::InputTag>("OpDetWaveformLabel") }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "icarus-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
  , m_metric_config{ pset.get<fhicl::ParameterSet>("PMTMetricConfig") }
  , pulseRecoManager()
{

  // Configure the redis metrics 
  sbndaq::GenerateMetricConfig( m_metric_config );

  // Configure the pedestal manager
  auto const ped_alg_pset = pset.get<fhicl::ParameterSet>("PedAlgoConfig");
  std::string pedAlgName = ped_alg_pset.get< std::string >("Name");
  if      (pedAlgName == "Edges")
    pedAlg = new pmtana::PedAlgoEdges(ped_alg_pset);
  else if (pedAlgName == "RollingMean")
    pedAlg = new pmtana::PedAlgoRollingMean(ped_alg_pset);
  else if (pedAlgName == "UB"   )
    pedAlg = new pmtana::PedAlgoUB(ped_alg_pset);
  else throw art::Exception(art::errors::UnimplementedFeature)
                    << "Cannot find implementation for "
                    << pedAlgName << " algorithm.\n";

  pulseRecoManager.SetDefaultPedAlgo(pedAlg);


  // Configure the ophitfinder manager
  auto const hit_alg_pset = pset.get<fhicl::ParameterSet>("HitAlgoConfig");
  std::string threshAlgName = hit_alg_pset.get< std::string >("Name");
  if      (threshAlgName == "Threshold")
    threshAlg = new pmtana::AlgoThreshold(hit_alg_pset);
  else if (threshAlgName == "SiPM")
    threshAlg = new pmtana::AlgoSiPM(hit_alg_pset);
  else if (threshAlgName == "SlidingWindow")
    threshAlg = new pmtana::AlgoSlidingWindow(hit_alg_pset);
  else if (threshAlgName == "FixedWindow")
    threshAlg = new pmtana::AlgoFixedWindow(hit_alg_pset);
  else if (threshAlgName == "CFD" )
    threshAlg = new pmtana::AlgoCFD(hit_alg_pset);
  else throw art::Exception(art::errors::UnimplementedFeature)
              << "Cannot find implementation for "
                    << threshAlgName << " algorithm.\n";

  pulseRecoManager.AddRecoAlgo(threshAlg);
  
}

//------------------------------------------------------------------------------------------------------------------


void sbndaq::CAENV1730Streams::clean() {

  //m_get_temperature.clear();

}


//-------------------------------------------------------------------------------------------------------------------

/*
int16_t sbndaq::CAENV1730Streams::Median(std::vector<int16_t> data, size_t n_adc) 
{
  // First we sort the array
  std::sort(data.begin(), data.end());

  // check for even case
  if (n_adc % 2 != 0)
    return data[n_adc / 2];

  return (data[(n_adc - 1) / 2] + data[n_adc / 2]) / 2.0;
}


double sbndaq::CAENV1730Streams::Median(std::vector<double> data, size_t n_adc) 
{
  // First we sort the array
  std::sort(data.begin(), data.end());

  // check for even case
  if (n_adc % 2 != 0)
    return data[n_adc / 2];

  return (data[(n_adc - 1) / 2] + data[n_adc / 2]) / 2.0;
}
*/


//------------------------------------------------------------------------------------------------------------------

/*
double sbndaq::CAENV1730Streams::RMS(std::vector<int16_t> const& data, size_t n_adc, int16_t baseline ) const
{
  double ret = 0;
  for (size_t i = 0; i < n_adc; i++) {
    ret += (data[i] - baseline) * (data[i] - baseline);
  }
  return sqrt(ret / n_adc);
}
*/

//------------------------------------------------------------------------------------------------------------------

template<typename T>
  T sbndaq::CAENV1730Streams::Median( std::vector<T> data ) const {

    std::nth_element( data.begin(), data.begin() + data.size()/2, data.end() );
    
    return data[ data.size()/2 ];

  }  

//------------------------------------------------------------------------------------------------------------------


void sbndaq::CAENV1730Streams::analyze(art::Event const & evt) {


  int level = 3; 

  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  artdaq::MetricMode rate = artdaq::MetricMode::Rate;

  std::string groupName = "PMT";


  // Now we look at the waveforms 
  art::Handle< std::vector<raw::OpDetWaveform> > opdetHandle;
  evt.getByLabel( m_opdetwaveform_tag, opdetHandle );

  if( opdetHandle.isValid() && !opdetHandle->empty() ) {

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
 

  // In case we need to clean something
  //clean();

  // Ronf for two seconds 
  //sleep(2); // Is it still necessary ? uncomment if so
   
}

DEFINE_ART_MODULE(sbndaq::CAENV1730Streams)
