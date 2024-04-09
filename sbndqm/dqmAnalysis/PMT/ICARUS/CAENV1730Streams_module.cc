////////////////////////////////////////////////////////////////////////
// Class:       CAENV1730Streams
// Plugin Type: analyzer
// File:        CAENV1730Streams_module.cc
// Author:      A. Scarpelli, with great help of G. Petrillo et al.
//		Partially reworked by M. Vicenzi (mvicenzi@bnl.gov)
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

  class CAENV1730Streams : public art::EDAnalyzer {

    public:

      explicit CAENV1730Streams(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  
      virtual void analyze(art::Event const & evt);
      void SendTemperatureMetrics(art::Event const & evt);  

    private:

      const unsigned int nChannelsPerBoard = 16;
      const unsigned int nBoards = 24;
      unsigned int nTotalChannels = nBoards*nChannelsPerBoard;
        
      art::InputTag m_opdetwaveform_tag;
      art::InputTag m_pmtdigitizer_tag;
      std::string m_redis_hostname;
        
      int m_redis_port;
      
      fhicl::ParameterSet m_metric_config;
      fhicl::ParameterSet m_board_metric_config;

      pmtana::PulseRecoManager pulseRecoManager;
      pmtana::PMTPulseRecoBase* threshAlg;
      pmtana::PMTPedestalBase*  pedAlg;

      FFTManager fftManager;

      template<typename T> T Median( std::vector<T> data ) const ;

  };

}

//------------------------------------------------------------------------------------------------------------------

sbndaq::CAENV1730Streams::CAENV1730Streams(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_opdetwaveform_tag{ pset.get<art::InputTag>("OpDetWaveformLabel") }
  , m_pmtdigitizer_tag{ pset.get<art::InputTag>("PMTDigitizerLabel") }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "icarus-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
  , m_metric_config{ pset.get<fhicl::ParameterSet>("PMTMetricConfig") }
  , m_board_metric_config{ pset.get<fhicl::ParameterSet>("PMTBoardMetricConfig") }
  , pulseRecoManager()
  , fftManager(0)
{

  // Configure the redis metrics 
  sbndaq::GenerateMetricConfig( m_metric_config );
  sbndaq::GenerateMetricConfig( m_board_metric_config );

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

void sbndaq::CAENV1730Streams::SendTemperatureMetrics(art::Event const & evt) {

  int level = 3; 
  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  
  art::Handle digitizerHandle = evt.getHandle<std::vector<pmtAnalysis::PMTDigitizerInfo>>(m_pmtdigitizer_tag);

  if( !digitizerHandle.isValid() || digitizerHandle->empty() ) {
    mf::LogError("sbndaq::CAENV1730Streams::analyze") 
      << "Data product '" << m_pmtdigitizer_tag.encode() << "' has no pmtAnalysis::PMTDigitizerInfo in it!\n";
    return;
  }
 
  // board information is repeated for each fragment in the event,
  // but temperatures evolve slowly... use only first instance!
  std::vector<size_t> board_counter;
 
  for ( auto const & digitizer : *digitizerHandle ) {
   
    size_t boardId = digitizer.getBoardId(); //this is the effective fragment_id
    std::string boardId_s = std::to_string(boardId);

    // skip if this board was already seen once in this event
    if( std::find( board_counter.begin(), board_counter.end(), boardId ) != board_counter.end() ) continue;
    board_counter.push_back(boardId);

    float maxT = 0;
    for(size_t  ch = 0; ch < digitizer.getNChannels(); ch++){
      float T = digitizer.getTemperature(ch);
      if( maxT < T ) maxT = T;
 
      size_t pmtID = ch + nChannelsPerBoard*boardId;
      std::string pmtID_s = std::to_string(pmtID);
     
      // send sigle-channel temperature
      sbndaq::sendMetric("PMT", pmtID_s, "temperature", T, level, mode);
    }

    // send board max temperature
    if( digitizer.getNChannels() > 1)
    	sbndaq::sendMetric("PMTBoard", boardId_s, "max_temperature", maxT, level, mode);
  }

}

//------------------------------------------------------------------------------------------------------------------

template<typename T>
  T sbndaq::CAENV1730Streams::Median( std::vector<T> data ) const {

    std::nth_element( data.begin(), data.begin() + data.size()/2, data.end() );
    
    return data[ data.size()/2 ];

  }  

//------------------------------------------------------------------------------------------------------------------


void sbndaq::CAENV1730Streams::analyze(art::Event const & evt) {

  // send PMT temperatures 
  SendTemperatureMetrics(evt);

  int level = 3; 
  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  artdaq::MetricMode rate = artdaq::MetricMode::Rate;
  std::string groupName = "PMT";

  // Now we look at the waveforms 
  art::Handle opdetHandle = evt.getHandle<std::vector<raw::OpDetWaveform>>(m_opdetwaveform_tag);

  if( !opdetHandle.isValid() || opdetHandle->empty() ) {
    mf::LogError("sbndaq::CAENV1730Streams::analyze") 
      << "Data product '" << m_opdetwaveform_tag.encode() << "' has no raw::OpDetWaveform in it!\n";
    return;
  }

  // Create a sample with only one waveforms per channel
  // we pick the longest waveform, which should contain the global trigger
  // if they have the same length, we pick the first

  std::map<unsigned int, unsigned int> unique_wfs; 
  unsigned int index = 0;

  for ( auto const & opdetwaveform : *opdetHandle ) {

    unsigned int const pmtId = opdetwaveform.ChannelNumber();
    std::size_t length = opdetwaveform.Waveform().size();

    if( unique_wfs.find(pmtId) == unique_wfs.end() ){
      unique_wfs.insert(std::make_pair(pmtId,index));
      index++;
      continue;
    }

    unsigned int seen_index = unique_wfs[pmtId];
    if( (*opdetHandle)[seen_index].Waveform().size() < length )
      unique_wfs[pmtId] = index;

    index++;
  }
  
  // Compute metrics for each channel from its waveform
  for( auto it = unique_wfs.begin(); it!=unique_wfs.end(); it++){
    
    std::string pmtId_s = std::to_string(it->first);

    // baseline computed as waveform median
    int16_t baseline = Median( (*opdetHandle)[it->second] );
	
    // baseline rms from nominal reco algorithm
    pulseRecoManager.Reconstruct( (*opdetHandle)[it->second] );
    std::vector<double>  pedestal_sigma = pedAlg->Sigma();
    double rms = Median( pedestal_sigma );

    if( pedestal_sigma.size() < 1 ) {    
      mf::LogWarning("sbndaq::CAENV1730Streams::analyze") 
          << "PmtId " << pmtId_s << " has " << pedestal_sigma.size() << " pedestal sigmas computed"
          << " (rms = " << rms << ")";
    }
    if( rms < 0.1 ) {
      mf::LogWarning("sbndaq::CAENV1730Streams::analyze") 
          << "PmtId " << pmtId_s << " with low rms " << rms << " ADC";
    }

    // rate estimation from number of pulses on waveform
    auto const& pulses = threshAlg->GetPulses();
    double npulses = (double)pulses.size();

    //std::cout << pmtId_s << " " << baseline << " " << rms << " " << npulses << std::endl;

    // Send the metrics 
    sbndaq::sendMetric(groupName, pmtId_s, "baseline", baseline, level, mode); // Send baseline information
    sbndaq::sendMetric(groupName, pmtId_s, "rms", rms, level, mode); // Send rms information
    sbndaq::sendMetric(groupName, pmtId_s, "rate", npulses, level, rate); // Send rms information
        
    // Now we send a copy of the waveform itself
    double tickPeriod = 0.002; // [us] 
    std::vector<raw::ADC_Count_t> adcs { (*opdetHandle)[it->second] };

    // send full waveform
    sbndaq::SendWaveform("snapshot:waveform:PMT:" + pmtId_s, adcs, tickPeriod);
    // send event meta
    sbndaq::SendEventMeta("snapshot:waveform:PMT:" + pmtId_s, evt);

    // Now we send a copy of the waveeform FFT
    size_t NADC = (*opdetHandle)[it->second].Waveform().size();
    double tickPeriodFFT = 1./tickPeriod; // [us], change this harcoding
    if ( fftManager.InputSize() != NADC ) fftManager.Set(NADC);
 
    // fill FFT
    for (size_t i=0; i<NADC; i++) {
      double *input = fftManager.InputAt(i);
      *input = (double)(*opdetHandle)[it->second].Waveform()[i] - baseline;
    }
    fftManager.Execute();

    std::vector<float> adcsFFT;
    double real=0, im=0;
    for(size_t k=0; k<fftManager.OutputSize(); k++){
      real=fftManager.ReOutputAt(k);
      im=fftManager.ImOutputAt(k);
      adcsFFT.push_back( std::hypot(real,im) );
    }

    // send waveform FFT
    sbndaq::SendWaveform("snapshot:fft:PMT:" + pmtId_s, adcsFFT, tickPeriodFFT);
    // send event meta
    sbndaq::SendEventMeta("snapshot:fft:PMT:" + pmtId_s, evt);

  }      

  if( unique_wfs.size() < nTotalChannels ) {
    mf::LogError("sbndaq::CAENV1730Streams::analyze") 
          << "Event has " << unique_wfs.size() << " channels, " << nTotalChannels << " expected.\n";
  }

  unique_wfs.clear();

}

DEFINE_ART_MODULE(sbndaq::CAENV1730Streams)
