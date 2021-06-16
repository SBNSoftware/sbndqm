////////////////////////////////////////////////////////////////////////
//
// 
////////////////////////////////////////////////////////////////////////

#include "messagefacility/MessageLogger/MessageLogger.h"
#include "CAENV1730WaveformAnalysis.hh"


sbndaq::CAENV1730WaveformAnalysis::CAENV1730WaveformAnalysis(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_pmtditigitizerinfo_tag{ pset.get<art::InputTag>("PMTDigitizerInfoLabel") }
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


double sbndaq::CAENV1730WaveformAnalysis::Median(std::vector<double> data, size_t n_adc) 
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
      
      auto findIt = std::find( m_unique_channels.begin(), m_unique_channels.end(), pmtId );

      if( findIt == m_unique_channels.end() ) {
		
        // We have never seen this channel before
        m_unique_channels.push_back( pmtId );
      
        std::string pmtId_s = std::to_string(pmtId);

        unsigned int const nsamples = opdetwaveform.size();

        int16_t baseline = Median(opdetwaveform, nsamples);
	
        pulseRecoManager.Reconstruct( opdetwaveform );
        std::vector<double>  pedestal_sigma = pedAlg->Sigma();
	
        double rms = Median(pedestal_sigma, pedestal_sigma.size());

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
	
	// We have already a waveform from this channel in our sample 
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

