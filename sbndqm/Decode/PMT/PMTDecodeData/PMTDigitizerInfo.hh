#ifndef SBNDQM_DECODE_PMT_PMTDECODEDATA_PMTDIGITIZERINFO_hh
#define SBNDQM_DECODE_PMT_PMTDECODEDATA_PMTDIGITIZERINFO_hh

#include <vector>
#include <array>
#include <string>

namespace pmtAnalysis 
{ 

	class PMTDigitizerInfo {

		public: 

			PMTDigitizerInfo();

			PMTDigitizerInfo( size_t const eff_fragment_id, 
							  unsigned int const time_tag,
				              uint64_t fragmentTimestamp, 
				              float temperature )
			: m_eff_fragment_id( eff_fragment_id )
			, m_time_tag( time_tag )
			, m_fragmentTimestamp( fragmentTimestamp )
			, m_digitizer_temperature( temperature )
			{};

			~PMTDigitizerInfo();

		private:

			size_t m_eff_fragment_id;
			
			unsigned int m_time_tag;
			
			uint64_t m_fragmentTimestamp;

			float m_digitizer_temperature;

	};

}

#endif