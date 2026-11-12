////////////////////////////////////////////////////////////////////////
//
// CAENV1740StreamsSBND
//
// X-ARAPUCA DQM analyzer, adapted from CAENV1730StreamsSBND (PMTs).
//
// Reads the std::vector<raw::OpDetWaveform> produced by the sbndcode
// SBNDXARAPUCADecoder (CAEN V1740B, 64 channels/board), extracts per-channel
// baseline / RMS / pulse rate, and sends them (plus waveform + FFT snapshots)
// to Redis through the sbndaq-online helpers.
//
// Decoder channel numbering: channel_id = slot*100 + board_channel
// (slots e.g. 7, 13, 16, 19). To keep the Redis metric instances contiguous this
// module remaps that to  xaraId = boardIndex*64 + board_channel  (0..255),
// where boardIndex is the position of the slot in the "BoardSlots" fcl list.
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

#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "messagefacility/MessageLogger/MessageLogger.h"

#include "sbndqm/dqmAnalysis/TPC/FFT.hh"
#include "sbndqm/dqmAnalysis/TPC/FFT.cc"


namespace sbndaq {

  class CAENV1740StreamsSBND : public art::EDAnalyzer {

    public:

      explicit CAENV1740StreamsSBND(fhicl::ParameterSet const & pset);

      virtual void analyze(art::Event const & evt) override;

    private:

      static constexpr unsigned int nChannelsPerBoard = 64; // V1740B

      art::InputTag fWaveformsModuleLabel;
      bool fVerbose;

      std::string m_redis_hostname;
      int m_redis_port;

      std::vector<unsigned int> m_board_slots; // slots, same order as the decoder's board_id_list
      unsigned int m_channel_id_factor;        // decoder: channel_id = slot*factor + board_channel
      unsigned int m_n_total_channels;

      double m_tick_period;                    // [us] (V1740: 0.016)
      std::vector<std::string> m_board_groups; // metric group per board, same order as m_board_slots

      fhicl::ParameterSet m_metric_config;

      pmtana::PulseRecoManager pulseRecoManager;
      pmtana::PMTPulseRecoBase* threshAlg;
      pmtana::PMTPedestalBase*  pedAlg;

      FFTManager fftManager;

      // Convert decoder channel_id to contiguous id and look up its metric group
      // (APSAIA for slot 7, ARARA for the others). Returns false if the slot is unknown.
      bool contiguousId(unsigned int channel_id, unsigned int & xaraId, std::string & group) const;

      template<typename T> T Median( std::vector<T> data ) const;
  };
}

//------------------------------------------------------------------------------------------------------------------

template<typename T>
  T sbndaq::CAENV1740StreamsSBND::Median( std::vector<T> data ) const {

    std::nth_element( data.begin(), data.begin() + data.size()/2, data.end() );

    return data[ data.size()/2 ];

  }

//------------------------------------------------------------------------------------------------------------------

bool sbndaq::CAENV1740StreamsSBND::contiguousId(unsigned int channel_id, unsigned int & xaraId, std::string & group) const {

  unsigned int const slot         = channel_id / m_channel_id_factor;
  unsigned int const boardChannel = channel_id % m_channel_id_factor;

  auto it = std::find( m_board_slots.begin(), m_board_slots.end(), slot );
  if( it == m_board_slots.end() || boardChannel >= nChannelsPerBoard ) return false;

  unsigned int const boardIndex = static_cast<unsigned int>( it - m_board_slots.begin() );

  xaraId = boardIndex * nChannelsPerBoard + boardChannel;
  group  = m_board_groups[boardIndex];
  return true;

}

//------------------------------------------------------------------------------------------------------------------

sbndaq::CAENV1740StreamsSBND::CAENV1740StreamsSBND(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , fWaveformsModuleLabel{ pset.get<art::InputTag>("OpDetWaveformLabel") }
  , fVerbose{ pset.get<bool>("Verbose", false) }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "sbnd-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
  , m_board_slots{ pset.get<std::vector<unsigned int>>("BoardSlots", {7, 13, 16, 19}) }
  , m_channel_id_factor{ pset.get<unsigned int>("ChannelIdFactor", 100) }
  , m_n_total_channels{ static_cast<unsigned int>(m_board_slots.size()) * nChannelsPerBoard }
  , m_tick_period{ pset.get<double>("TickPeriod", 0.016) }
  , m_board_groups{ pset.get<std::vector<std::string>>("BoardGroups", {"APSAIA", "ARARA", "ARARA", "ARARA"}) }
  , m_metric_config{ pset.get<fhicl::ParameterSet>("XARAPUCAMetricConfig") }
  , pulseRecoManager()
  , fftManager(0)
{

  if( m_board_groups.size() != m_board_slots.size() ) {
    throw art::Exception(art::errors::Configuration)
      << "BoardGroups has " << m_board_groups.size() << " entries but BoardSlots has "
      << m_board_slots.size() << "; they must match one-to-one.\n";
  }

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

void sbndaq::CAENV1740StreamsSBND::analyze(art::Event const & evt) {

  int level = 3;

  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  artdaq::MetricMode rate = artdaq::MetricMode::Rate;

  // ========== GETS THE WAVEFORMS FROM THE EVENT ==========
  if (fVerbose) std::cout << "\nAccessing to " << fWaveformsModuleLabel << " label." << std::endl;

  art::Handle<std::vector<raw::OpDetWaveform>> opdetHandle;
  evt.getByLabel(fWaveformsModuleLabel, opdetHandle);

  // The art::Handle object is not valid.
  if (!opdetHandle.isValid()) {
    if (fVerbose) std::cout << "\nWaveforms handle not valid for " << fWaveformsModuleLabel << "." << std::endl;
    if (fVerbose) std::cout << "\n" << opdetHandle.whyFailed() << std::endl;
    return;
  }

  // The art::Handle object is empty.
  if (opdetHandle->empty()) {
    if (fVerbose) std::cout << "\nWaveforms handle is empty." << std::endl;
    return;
  }

  // The art::Handle object is valid and not empty
  if (fVerbose) std::cout << "Valid ( " << opdetHandle->size() << " objects)." << std::endl;

  // Only one waveform per channel is sent per event
  std::vector<unsigned int> uniqueChannels;

  for ( auto const & opdetwaveform : *opdetHandle ) {

    if( opdetwaveform.empty() ) { continue; }

    unsigned int xaraId = 0;
    std::string group;
    if( !contiguousId( opdetwaveform.ChannelNumber(), xaraId, group ) ) {
      mf::LogWarning("sbndaq::CAENV1740StreamsSBND::analyze")
        << "Skipping unknown channel number " << opdetwaveform.ChannelNumber()
        << " (slot not in BoardSlots)\n";
      continue;
    }

    if( std::find( uniqueChannels.begin(), uniqueChannels.end(), xaraId ) != uniqueChannels.end() ) { continue; }
    uniqueChannels.push_back( xaraId );

    std::string const xaraId_s = std::to_string( xaraId );

    int16_t baseline = Median( opdetwaveform );

    pulseRecoManager.Reconstruct( opdetwaveform );
    std::vector<double> pedestal_sigma = pedAlg->Sigma();

    double rms = Median( pedestal_sigma );

    auto const& pulses = threshAlg->GetPulses();
    double npulses = (double)pulses.size();

    // Send the metrics
    sbndaq::sendMetric(group, xaraId_s, "baseline", baseline, level, mode);
    sbndaq::sendMetric(group, xaraId_s, "rms",      rms,      level, mode);
    sbndaq::sendMetric(group, xaraId_s, "rate",     npulses,  level, rate);

    // Send a copy of the waveform
    std::vector<std::vector<raw::ADC_Count_t>> adcs {opdetwaveform};
    std::vector<int> start { 0 };

    sbndaq::SendSplitWaveform("snapshot:waveform:" + group + ":" + xaraId_s, adcs, start, m_tick_period);

    // Send a copy of the waveform FFT
    size_t NADC = opdetwaveform.Waveform().size();
    double tickPeriodFFT = 1./m_tick_period;
    fftManager.Set(NADC);

    for (size_t i = 0; i < NADC; i++) {
      double *input = fftManager.InputAt(i);
      *input = (double) opdetwaveform.Waveform()[i];
    }
    fftManager.Execute();

    std::vector<float> adcsFFT;
    for(size_t k = 0; k < fftManager.OutputSize(); k++){
      adcsFFT.push_back( std::hypot( fftManager.ReOutputAt(k), fftManager.ImOutputAt(k) ) );
    }

    sbndaq::SendWaveform("snapshot:fft:" + group + ":" + xaraId_s, adcsFFT, tickPeriodFFT);

    // EventMeta
    sbndaq::SendEventMeta("snapshot:waveform:" + group + ":" + xaraId_s, evt);
    sbndaq::SendEventMeta("snapshot:fft:" + group + ":" + xaraId_s, evt);

  } // for waveforms

  if( uniqueChannels.size() < m_n_total_channels ) {

    mf::LogError("sbndaq::CAENV1740StreamsSBND::analyze")
      << "Event has " << uniqueChannels.size() << " channels, " << m_n_total_channels << " expected.\n";

  }

}

DEFINE_ART_MODULE(sbndaq::CAENV1740StreamsSBND)
