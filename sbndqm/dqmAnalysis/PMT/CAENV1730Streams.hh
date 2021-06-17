#ifndef SBNDQM_DQMANALYSIS_PMT_CAENV1730STREAMS_hh
#define SBNDQM_DQMANALYSIS_PMT_CAENV1730STREAMS_hh

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

	class CAENV1730Streams : public art::EDAnalyzer {

		public:

  			explicit CAENV1730Streams(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  			virtual ~CAENV1730Streams();
  
  			virtual void analyze(art::Event const & evt);
  
		private:

			const unsigned int nChannelsPerBoard = 16;
  			
  			art::InputTag m_pmtditigitizerinfo_tag;
  			art::InputTag m_opdetwaveform_tag;

  			std::string m_redis_hostname;
  			
  			int m_redis_port;

  			double stringTime = 0.0;

  			fhicl::ParameterSet m_metric_config;

  			pmtana::PulseRecoManager pulseRecoManager;
  			pmtana::PMTPulseRecoBase* threshAlg;
  			pmtana::PMTPedestalBase*  pedAlg;

  			std::map<unsigned int, float> m_get_temperature;

  			void clean();

  			int16_t Median(std::vector<int16_t> data, size_t n_adc);
  			double Median(std::vector<double> data, size_t n_adc);
 			
 			double RMS(std::vector<int16_t> data, size_t n_adc, int16_t baseline );
  			
  			int16_t Min(std::vector<int16_t> data, size_t n_adc, int16_t baseline );

	};


}

#endif