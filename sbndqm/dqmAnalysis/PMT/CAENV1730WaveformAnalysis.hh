#ifndef CAENV1730WaveformAnalysis_hh
#define CAENV1730WaveformAnalysis_hh

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
#include "sbndqm/Decode/PMT/PMTDigitizerInfo.hh"

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

  			int16_t Median(std::vector<int16_t> data, size_t n_adc);
 			
 			double RMS(std::vector<int16_t> data, size_t n_adc, int16_t baseline );
  			
  			int16_t Min(std::vector<int16_t> data, size_t n_adc, int16_t baseline );

	};


}

#endif