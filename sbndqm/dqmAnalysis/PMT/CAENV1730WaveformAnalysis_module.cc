////////////////////////////////////////////////////////////////////////
// Class:       CAENV1730WaveformAna
// Module Type: analyzer
// File:        CAENV1730WaveformAna_module.cc
// Description: Makes a tree with waveform information.
////////////////////////////////////////////////////////////////////////
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/Exception.h"
#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "artdaq-core/Data/Fragment.hh"

#include "sbndaq-decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

//#include "art/Framework/Services/Optional/TFileService.h" //before art_root_io transition
//#include "art_root_io/TFileService.h"
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
  class CAENV1730WaveformAnalysis;
}
/*****/
class sbndaq::CAENV1730WaveformAnalysis : public art::EDAnalyzer {
public:
  explicit CAENV1730WaveformAnalysis(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~CAENV1730WaveformAnalysis();
  
  virtual void analyze(art::Event const & evt);
  
private:
  
  //TNtuple* nt_header; //Ntuple header
  //TNtuple* nt_wvfm;
  std::vector< std::vector<uint16_t> >  fWvfmsVec;
  std::string fRedisHostname;
  int         fRedisPort;

  double stringTime = 0.0;

  int16_t Median(std::vector<int16_t> data, size_t n_adc);
  double RMS(std::vector<int16_t> data, size_t n_adc, int16_t baseline );
  int16_t Min(std::vector<int16_t> data, size_t n_adc, int16_t baseline );

};
//Define the constructor
sbndaq::CAENV1730WaveformAnalysis::CAENV1730WaveformAnalysis(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset),
   fRedisHostname(pset.get<std::string>("RedisHostname","icarus-db01")),
  fRedisPort(pset.get<int>("RedisPort",6379))
{


  //  art::ServiceHandle<art::TFileService> tfs; //pointer to a file named tfs
  //  nt_header = tfs->make<TNtuple>("nt_header","CAENV1730 Header Ntuple","art_ev:caen_ev:caen_ev_tts");
  //  nt_wvfm = tfs->make<TNtuple>("nt_wvfm","Waveform information Ntuple","art_ev:caen_ev:caen_ev_tts:ch:ped:rms:temp");

  sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));

}

sbndaq::CAENV1730WaveformAnalysis::~CAENV1730WaveformAnalysis()
{
}

int16_t sbndaq::CAENV1730WaveformAnalysis::Median(std::vector<int16_t> data, size_t n_adc) 
{
  // First we sort the array
  std::sort(data.begin(), data.end());

  // check for even case
  if (n_adc % 2 != 0)
    return data[n_adc / 2];

  return (data[(n_adc - 1) / 2] + data[n_adc / 2]) / 2.0;
}


double sbndaq::CAENV1730WaveformAnalysis::RMS(std::vector<int16_t> data, size_t n_adc, int16_t baseline )
{
  double ret = 0;
  for (size_t i = 0; i < n_adc; i++) {
    ret += (data[i] - baseline) * (data[i] - baseline);
  }
  return sqrt(ret / n_adc);
}

int16_t sbndaq::CAENV1730WaveformAnalysis::Min(std::vector<int16_t> data, size_t n_adc, int16_t baseline )
{
  int16_t min = baseline;
  for (size_t i = 0; i < n_adc; i++) {
    if( data[i] < min ) { min = data[i]; } ;
  }
  return min;
}

void sbndaq::CAENV1730WaveformAnalysis::analyze(art::Event const & evt)
{
  

  art::Handle< std::vector<raw::OpDetWaveform> > OpdetHandle; 
  evt.getByLabel("daq", OpdetHandle); 
  //  art::ServiceHandle< art::TFileService > tfs;
  
  
  if (!OpdetHandle.isValid()) return;
  
 
 int level = 0;
 artdaq::MetricMode mode = artdaq::MetricMode::Average; 
 std::string group_name = "PMT";
 
 std::vector<raw::OpDetWaveform> const& raw_opdet_vector(*OpdetHandle);
 
 for (size_t idx = 0; idx < OpdetHandle->size(); ++idx){
   auto const& rd = raw_opdet_vector[idx];
   //
   std::string channel_no = std::to_string(rd.ChannelNumber());

   int16_t baseline = Median(rd, rd.size());
   double rms = RMS(rd, rd.size(), baseline);
   //int16_t min = Min(rd, rd.size(), baseline);

   // Skip if the waveform has some signal
   //if( baseline - min > 180 ){ rms =0; baseline=0; } // this waveform has a signal too large and should be skipped ( or not counted )
   
   sbndaq::sendMetric(group_name, channel_no, "baseline", baseline, level, mode); // Send baseline information
   sbndaq::sendMetric(group_name, channel_no, "rms", rms, level, mode); // Send rms information
  
   // send each waveform
   double tick_period = 2.; // [us] 
   std::vector<std::vector<raw::ADC_Count_t>> adcs {rd};
   std::vector<int> start {(int)(rd.TimeStamp() / tick_period) /*convert us -> TDC */};

   sbndaq::SendSplitWaveform("snapshot:waveform:PMT:" + std::to_string(rd.ChannelNumber()), adcs, start, tick_period);
   sbndaq::SendEventMeta("snapshot:waveform:PMT:" + std::to_string(rd.ChannelNumber()), evt);

 }// end loop over OpdetHandle	    

 //now take a nap for 1 sec
 sleep(2);
   
}

DEFINE_ART_MODULE(sbndaq::CAENV1730WaveformAnalysis)

