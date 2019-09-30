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

#include "../../MetricManagerShim/MetricManager.hh"
#include "../../MetricConfig/ConfigureRedis.hh"
#include "sbndaq-redis-plugin/Utilities.h"

//#include "art/Framework/Services/Optional/TFileService.h" //before art_root_io transition
#include "art_root_io/TFileService.h"
#include "TH1F.h"
#include "TNtuple.h"
#include <algorithm>
#include <cassert>
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
  
  TNtuple* nt_header; //Ntuple header
  TNtuple* nt_wvfm;
  std::vector< std::vector<uint16_t> >  fWvfmsVec;
  std::string fRedisHostname;
  int         fRedisPort;

  double stringTime = 0.0;
  redisContext* context;
  void makeStrings(raw::OpDetWaveform const&);
  
};
//Define the constructor
sbndaq::CAENV1730WaveformAnalysis::CAENV1730WaveformAnalysis(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset),
   fRedisHostname(pset.get<std::string>("RedisHostname","icarus-db01")),
  fRedisPort(pset.get<int>("RedisPort",6379))
{


  art::ServiceHandle<art::TFileService> tfs; //pointer to a file named tfs
  nt_header = tfs->make<TNtuple>("nt_header","CAENV1730 Header Ntuple","art_ev:caen_ev:caen_ev_tts");
  nt_wvfm = tfs->make<TNtuple>("nt_wvfm","Waveform information Ntuple","art_ev:caen_ev:caen_ev_tts:ch:ped:rms:temp");

 context =  sbndaq::Connect2Redis(fRedisHostname,fRedisPort);
  sbndqm::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  sbndqm::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));

}

void sbndaq::CAENV1730WaveformAnalysis::makeStrings(raw::OpDetWaveform const& rd){ 

   redisAppendCommand(context, "DEL snapshot:waveform:PMT:%i", rd.ChannelNumber());

   size_t buffer_len = rd.size() * 10 + 50;
   char *buffer = new char[buffer_len];
   
   // print in the base of the command
   size_t print_len = sprintf(buffer, "RPUSH snapshot:waveform:PMT:%i", rd.ChannelNumber());
   char *buffer_index = buffer + print_len;

   // throw in all of the data points
   for (size_t tick = 0; tick < rd.size(); tick++) {
     print_len += sprintf(buffer_index, " %i", rd[tick]); 
     buffer_index = buffer + print_len;
     if (print_len >= buffer_len - 1) {
       std::cerr << "ERROR: BUFFER OVERFLOW IN WAVEFORM DATA" << std::endl;
       std::exit(1);
     }
   }
   // std::cout<<Wave value before Redis is <<rd.ADCs()
   // null terminate the string
   *buffer_index = '\0';
   redisAppendCommand(context, buffer);
  // sendString.Stop();
 //  sSum = sSum + sendString.RealTime();
//   if (rd.Channel() == 575) {
//     std::cout<<" Time to create the buffer for redis is "<<sSum <<" seconds."<<std::endl;
//   }
//   redisString.Start();
   redisGetReply(context,NULL);
   redisGetReply(context,NULL);                                                                                                                         
   // delete the buffer                                                                                                                         
   delete buffer;
 //  redisString.Stop();
 
   //return time;
}

sbndaq::CAENV1730WaveformAnalysis::~CAENV1730WaveformAnalysis()
{
}
void sbndaq::CAENV1730WaveformAnalysis::analyze(art::Event const & evt)
{
  

  art::Handle< std::vector<raw::OpDetWaveform> > OpdetHandle; 
  evt.getByLabel("daq", OpdetHandle); 
  art::ServiceHandle< art::TFileService > tfs;
  
  
  if (!OpdetHandle.isValid()) return;
 
  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;
  
 
 int level = 0;
 artdaq::MetricMode mode = artdaq::MetricMode::Average;
 std::string group_name = "PMT";
 
 std::vector<raw::OpDetWaveform> const& raw_opdet_vector(*OpdetHandle);
 
 for (size_t idx = 0; idx < OpdetHandle->size(); ++idx){
  auto const& rd = raw_opdet_vector[idx];
  //
  std::string channel_no = std::to_string(rd.ChannelNumber());
  sbndqm::sendMetric(group_name, channel_no, "rms", idx*1, level, mode);
  
  makeStrings(rd); 

//for (auto const& waveform : *OpdetHandle){

 double firstWaveformTime = rd.TimeStamp();

 int channel = rd.ChannelNumber();
 std::cout<<"waveform size is"<< rd.size()<<"\n";
 std::cout<<"channel number is"<< channel<<"\n";
  std::stringstream histName;
      histName << "event_"      << evt.id().event() 
               << "_opchannel_" << channel;        
      // Increase counter for number of waveforms on this optical channel
    
        TH1D *waveformHist = tfs->make< TH1D >(histName.str().c_str(),TString::Format(";t - %f (#mus);",firstWaveformTime),rd.size(), 0, rd.size());
     // Copy values from the waveform into the histogram
      for (size_t tick = 0; tick < rd.size(); tick++){
        waveformHist->SetBinContent(tick + 1, rd[tick]);
	//std::cout<<"waveform value is"<<waveform[tick]<<"\n";
//	}


}

  
 }	    

   
}
DEFINE_ART_MODULE(sbndaq::CAENV1730WaveformAnalysis)
//this is where the name is specified
