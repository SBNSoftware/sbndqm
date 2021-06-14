#ifndef _sbnddaq_analysis_PMTWaveform
#define _sbnddaq_analysis_PMTWaveform

#include <vector>
#include <array>
#include <string>

#include "lardataobj/RawData/OpDetWaveform.h"

namespace pmtAnalysis 
{ 

	class PMTWaveform: public raw::OpDetWaveform {

		public: 

			PMTWaveform(){};

			PMTWaveform( double time,
						 unsigned int   chan,
   	                     size_t len) 
			: raw::OpDetWaveform{ time, chan, len }
			{};

			void setPedestalConfiguration( size_t pedestal_config );

		private: 

			size_t m_pedestal_config=0;
	};

}

#endif