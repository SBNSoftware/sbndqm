#ifndef SBNDQM_DQMANALYSIS_PMT_CAENV1730WAVEFORMANALYSIS_hh
#define SBNDQM_DQMANALYSIS_PMT_CAENV1730WaveformAnalysis_hh

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/TableAs.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Table.h"
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

#include "TH1F.h"
#include "TNtuple.h"
#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>

namespace sbndaq {

	class CAENV1730WaveformAnalysis : public art::EDAnalyzer {

		public:

			struct Config 
      		{

        		using Name = fhicl::Name;

        		using Comment = fhicl::Comment;

        		fhicl::Atom<art::InputTag> OpDetWaveformLabel {
          			Name("OpDetWaveformLabel"), 
          			Comment("data products lables for the OpDetWaveform data product"), 
          			art::InputTag{ "daqPMT" }
        		};

        		fhicl::Atom<art::InputTag> PMTDigitizerInfoLabel {
          			Name("PMTDigitizerInfoLabel"), 
          			Comment("data products lables for the PMTDigitizerInfo data product"), 
          			art::InputTag{ "daqPMT" }
        		};

   				fhicl::Atom<std::string> RedisHostname {
          			Name("RedisHostname"), 
          			Comment("Redis database hostname"), 
          			std::string{ "icarus-db01" }
        		};

        		fhicl::Atom<int> RedisPort {
          			Name("RedisPort"), 
          			Comment("Redis database hostname"), 
          			int{ 6379 }
        		};

        		fhicl::Table<fhicl::ParameterSet> MetricConfig {
        			Name("MetricConfig"),
        			Comment( "Configuration of the redis metrics" ),
        		};

        		fhicl::Table<fhicl::ParameterSet> PedAlgoConfig {
        			Name("PedAlgoConfig"),
        			Comment( "Configuration of the pedestal removal algorithm" ),
        		};

        		fhicl::Table<fhicl::ParameterSet> HitAlgoConfig {
        			Name("HitAlgoConfig"),
        			Comment( "Configuration of the ophit finding algorithm" ),
        		};

      		};

      		using Parameters = art::EDAnalyzer::Table<Config>;
  		
  			explicit CAENV1730WaveformAnalysis(Parameters const & params); // explicit doesn't allow for copy initialization
  			virtual ~CAENV1730WaveformAnalysis();
  
  			virtual void analyze(art::Event const & evt);
  
		private:
  			
  			art::InputTag m_pmtditigitizerinfo_tag;
  			art::InputTag m_opdetwaveform_tag;

  			std::string m_redis_hostname;
  			
  			int m_redis_port;

  			double stringTime = 0.0;

  			pmtana::PulseRecoManager pulseRecoManager;
  			pmtana::PMTPulseRecoBase* threshAlg;
  			pmtana::PMTPedestalBase*  pedAlg;

  			int16_t Median(std::vector<int16_t> data, size_t n_adc);
  			double Median(std::vector<double> data, size_t n_adc);
 			
 			double RMS(std::vector<int16_t> data, size_t n_adc, int16_t baseline );
  			
  			int16_t Min(std::vector<int16_t> data, size_t n_adc, int16_t baseline );

	};


}

#endif