////////////////////////////////////////////////////////////////////////
//
// 
////////////////////////////////////////////////////////////////////////

#include "messagefacility/MessageLogger/MessageLogger.h"
#include "CAENV1730WaveformAnalysis.hh"


sbndaq::CAENV1730WaveformAnalysis::CAENV1730WaveformAnalysis(Parameters const & params)
  : EDAnalyzer(params)
  , m_pmtditigitizerinfo_tag{ params().PMTDigitizerInfoLabel() }
  , m_opdetwaveform_tag{ params().OpDetWaveformLabel() }
  , m_redis_hostname{ params().RedisHostname() }
  , m_redis_port{ params().RedisPort() }
{

  sbndaq::GenerateMetricConfig( params().MetricConfig() );

}

//------------------------------------------------------------------------------------------------------------------


sbndaq::CAENV1730WaveformAnalysis::~CAENV1730WaveformAnalysis(){};


//------------------------------------------------------------------------------------------------------------------



int16_t sbndaq::CAENV1730WaveformAnalysis::Median(std::vector<int16_t> data, size_t n_adc) 
{
  // First we sort the array
  std::sort(data.begin(), data.end());

  // check for even case
  if (n_adc % 2 != 0)
    return data[n_adc / 2];

  return (data[(n_adc - 1) / 2] + data[n_adc / 2]) / 2.0;
}


//------------------------------------------------------------------------------------------------------------------


double sbndaq::CAENV1730WaveformAnalysis::RMS(std::vector<int16_t> data, size_t n_adc, int16_t baseline )
{
  double ret = 0;
  for (size_t i = 0; i < n_adc; i++) {
    ret += (data[i] - baseline) * (data[i] - baseline);
  }
  return sqrt(ret / n_adc);
}


//------------------------------------------------------------------------------------------------------------------


int16_t sbndaq::CAENV1730WaveformAnalysis::Min(std::vector<int16_t> data, size_t n_adc, int16_t baseline )
{
  int16_t min = baseline;
  for (size_t i = 0; i < n_adc; i++) {
    if( data[i] < min ) { min = data[i]; } ;
  }
  return min;
}

//------------------------------------------------------------------------------------------------------------------

void sbndaq::CAENV1730WaveformAnalysis::analyze(art::Event const & evt) {

  // Get the event timestamp

  
  // Do some stuff with the digitizers


  // Now we look at the waveforms 

  int level = 3; 

  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  std::string groupName = "PMT";

  auto opdetHandle = evt.getValidHandle< std::vector<raw::OpDetWaveform> >( m_opdetwaveform_tag );

  if( opdetHandle->size() > 0 ) {

    // Create a sample with only one waveforms per channel

    std::vector<unsigned int> m_unique_channels; 


    for ( auto const & opdetwaveform : *opdetHandle ) {

      unsigned int const pmtId = opdetwaveform.ChannelNumber();


      auto find = std::find( m_unique_channels.begin(), m_unique_channels.end(), pmtId );

      if( find != m_unique_channels.end() ) {

        // We have never seen this channel before
        m_unique_channels.push_back( pmtId );
      
        std::string pmtId_s = std::to_string(pmtId);

        unsigned int const nsamples = opdetwaveform.size();

        int16_t baseline = Median(opdetwaveform, nsamples);
        double rms = RMS(opdetwaveform, nsamples, baseline);


        sbndaq::sendMetric(groupName, pmtId_s, "baseline", baseline, level, mode); // Send baseline information
        sbndaq::sendMetric(groupName, pmtId_s, "rms", rms, level, mode); // Send rms information


        // Now we send a copy of the waveforms 
        double tickPeriod = 2.; // [us] 
        std::vector<std::vector<raw::ADC_Count_t>> adcs {opdetwaveform};
        std::vector<int> start { 0 }; // We are considreing each waveform independent for now 

        sbndaq::SendSplitWaveform("snapshot:waveform:PMT:" + pmtId_s, adcs, start, tickPeriod);
        sbndaq::SendEventMeta("snapshot:waveform:PMT:" + pmtId_s, evt);
  
      } 

      else {

        // We have already this waveform in our sample 
        continue;
      }

    } // for      

    if( m_unique_channels.size() < 360 ) {

         mf::LogError("sbndaq::CAENV1730WaveformAnalysis::analyze") 
          << "Event has less than 360 waveforms'\n'";

    }

  }

  else {

     mf::LogError("sbndaq::CAENV1730WaveformAnalysis::analyze") 
          << "No raw::OpDetWaveform data product found with the used label! '\n'";

  }
 
 // Ronf for two seconds 
 //sleep(2); // Is it still necessary ? uncomment if so
   
}

DEFINE_ART_MODULE(sbndaq::CAENV1730WaveformAnalysis)

