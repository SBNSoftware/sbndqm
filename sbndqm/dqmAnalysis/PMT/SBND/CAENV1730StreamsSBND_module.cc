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

#include "sbndqm/dqmAnalysis/TPC/FFT.hh"
#include "sbndqm/dqmAnalysis/TPC/FFT.cc"


namespace sbndaq {

  class CAENV1730StreamsSBND : public art::EDAnalyzer {

    public:

        explicit CAENV1730StreamsSBND(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  
        virtual void analyze(art::Event const & evt) override;
  
    private:

      static constexpr unsigned int nChannelsPerBoard = 16;
      static constexpr unsigned int nBoards = 24;
      unsigned int nTotalChannels = nBoards*nChannelsPerBoard;
        
        art::InputTag m_opdetwaveform_tag;

        std::string m_redis_hostname;
        
        int m_redis_port;

        double stringTime = 0.0;

        fhicl::ParameterSet m_metric_config;

        pmtana::PulseRecoManager pulseRecoManager;
        pmtana::PMTPulseRecoBase* threshAlg;
        pmtana::PMTPedestalBase*  pedAlg;

        FFTManager fftManager;

 
        void clean();

        template<typename T> T Median( std::vector<T> data ) const ;
  };
}

//------------------------------------------------------------------------------------------------------------------


void sbndaq::CAENV1730StreamsSBND::clean() {

  //m_get_temperature.clear();

}

//------------------------------------------------------------------------------------------------------------------

template<typename T>
  T sbndaq::CAENV1730StreamsSBND::Median( std::vector<T> data ) const {

    std::nth_element( data.begin(), data.begin() + data.size()/2, data.end() );
    
    return data[ data.size()/2 ];

  }  

//------------------------------------------------------------------------------------------------------------------

sbndaq::CAENV1730StreamsSBND::CAENV1730StreamsSBND(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_opdetwaveform_tag{ pset.get<art::InputTag>("OpDetWaveformLabel") }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "sbnd-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
  , m_metric_config{ pset.get<fhicl::ParameterSet>("PMTMetricConfig") }
  , pulseRecoManager()
  , fftManager(0)
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

void sbndaq::CAENV1730StreamsSBND::analyze(art::Event const & evt) {

  int level = 3; 

  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  artdaq::MetricMode rate = artdaq::MetricMode::Rate;

  std::string groupName = "PMT";


  // Now we look at the waveforms 
  art::Handle opdetHandle
   = evt.getHandle<std::vector<raw::OpDetWaveform>>( m_opdetwaveform_tag);

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

      std::cout << pmtId << " " << baseline << " " << rms << " " << npulses << std::endl;

      // Send the metrics 
      sbndaq::sendMetric(groupName, pmtId_s, "baseline", baseline, level, mode); // Send baseline information
      sbndaq::sendMetric(groupName, pmtId_s, "rms", rms, level, mode); // Send rms information
      sbndaq::sendMetric(groupName, pmtId_s, "rate", npulses, level, rate); // Send rate information
          

      // Now we send a copy of the waveforms
      double tickPeriod = 0.002; // [us]
      std::vector<std::vector<raw::ADC_Count_t>> adcs {opdetwaveform};
      std::vector<int> start { 0 }; // We are considreing each waveform independent for now

      // send waveform
      sbndaq::SendSplitWaveform("snapshot:waveform:PMT:" + pmtId_s, adcs, start, tickPeriod);


      // Now we send a copy of the waveeform FFT
      size_t NADC = opdetwaveform.Waveform().size();
      double tickPeriodFFT = 1./tickPeriod; // [us], change this harcoding
      //std::cout<<"Waveform size: "<<NADC<<" tickPeriodFFT="<<tickPeriodFFT<<std::endl;
      fftManager.Set(NADC);
      
      // fill FFT
      for (size_t i = 0; i < NADC; i ++) {
	double *input = fftManager.InputAt(i);
        *input = (double) opdetwaveform.Waveform()[i];
      }
      fftManager.Execute();

      std::vector<float> adcsFFT;
      double real=0, im=0;
      for(size_t k=0; k<fftManager.OutputSize();k++){
        real=fftManager.ReOutputAt(k);
        im=fftManager.ImOutputAt(k);
        adcsFFT.push_back( std::hypot(real,im) );
      }

      // send waveform FFT
      sbndaq::SendWaveform("snapshot:fft:PMT:" + pmtId_s, adcsFFT, tickPeriodFFT);
      
      // send EventMeta
      sbndaq::SendEventMeta("snapshot:waveform:PMT:" + pmtId_s, evt);


    } // for      

    if( m_unique_channels.size() < nTotalChannels ) {

         mf::LogError("sbndaq::CAENV1730StreamsSBND::analyze") 
          << "Event has " << m_unique_channels.size() << " channels, " << nTotalChannels << " expected.\n";
    }

  }

  else {

     mf::LogError("sbndaq::CAENV1730StreamsSBND::analyze") 
          << "Data product '" << m_opdetwaveform_tag.encode() << "' has no raw::OpDetWaveform in it!\n";

  }
 

  // In case we need to clean something
  //clean();

  // Ronf for two seconds 
  //sleep(2); // Is it still necessary ? uncomment if so
   
}

DEFINE_ART_MODULE(sbndaq::CAENV1730StreamsSBND)
