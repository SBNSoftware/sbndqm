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
  
};
//Define the constructor
sbndaq::CAENV1730WaveformAnalysis::CAENV1730WaveformAnalysis(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{
  art::ServiceHandle<art::TFileService> tfs; //pointer to a file named tfs
  nt_header = tfs->make<TNtuple>("nt_header","CAENV1730 Header Ntuple","art_ev:caen_ev:caen_ev_tts");
  nt_wvfm = tfs->make<TNtuple>("nt_wvfm","Waveform information Ntuple","art_ev:caen_ev:caen_ev_tts:ch:ped:rms:temp");

  sbndqm::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  sbndqm::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));

}
sbndaq::CAENV1730WaveformAnalysis::~CAENV1730WaveformAnalysis()
{
}
void sbndaq::CAENV1730WaveformAnalysis::analyze(art::Event const & evt)
{
  
  //art::EventNumber_t eventNumber = evt.event();
  
 //   art::Handle< std::vector<artdaq::Fragment> > rawFragHandle; // it is a pointer to a vector of art fragments
 //   evt.getByLabel("daq","CAENV1730", rawFragHandle); // it says how many fragments are in an event

  // 
  art::Handle< std::vector<raw::OpDetWaveform> > OpdetHandle; 
  evt.getByLabel("daq", OpdetHandle); 
  
  if (!OpdetHandle.isValid()) return;
 // if (rawFragHandle.isValid()) return;
  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;
  
 /* std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()
            << ", event " << eventNumber << " has " << OpdetHandle->size()
            << " fragment(s)." << std::endl;
 */
 
 int level = 0;
 artdaq::MetricMode mode = artdaq::MetricMode::Average;
 std::string group_name = "PMT";
 
 std::vector<raw::OpDetWaveform> const& raw_opdet_vector(*OpdetHandle);
 // define vector that is the address of the raw_opdet_vector (pointer to OpdetHandle
 // 
 // actual rms
 // time stamp *
 // pedestal -> fourier transformations

 // adc count
 // waveform length *

 // adc count per channel / (integral of waveform) -> average pulse height

 for (size_t idx = 0; idx < OpdetHandle->size(); ++idx){
  // std::cout<< "Wveform is" << OpdetHandle[idx] << "\n";
  //std::string channel_no = std::to_string();
  auto const& rd = raw_opdet_vector[idx];
  //
  std::string channel_no = std::to_string(rd.ChannelNumber());
  sbndqm::sendMetric(group_name, channel_no, "rms", idx*1, level, mode);
  
 }	    

 //  for (size_t idx = 0; idx < OpdetHandle->size(); ++idx)
  
 // for (size_t idx = 0; idx < rawFragHandle->size(); ++idx) { // loop over the fragments of an event
    
  //  const auto& frag((*OpdetHandle)[idx]); // use this fragment as a refernce to the same data
/*    const auto& frag((*rawFragHandle)[idx]);
    sbndaq::CAENV1730Fragment bb(frag);
    
    auto const* md = bb.Metadata();
    CAENV1730Event const* event_ptr = bb.Event();
    CAENV1730EventHeader header = event_ptr->Header;
    
    std::cout << "\tFrom header, event counter is " << header.eventCounter << std::endl;
    std::cout << "\tFrom header, triggerTimeTag is " << header.triggerTimeTag << std::endl;
    
    nt_header->Fill(eventNumber,header.eventCounter,header.triggerTimeTag);
    
    
    //get the number of 32-bit words from the header
    size_t const& ev_size(header.eventSize);
    
    size_t nChannels = md->nChannels; //fixme
    fWvfmsVec.resize(nChannels);
    
    //use that to get the number of 16-bit words for each channel
    size_t n_samples = (ev_size - sizeof(CAENV1730EventHeader)/sizeof(uint32_t))*2/nChannels;
    const uint16_t* data = reinterpret_cast<const uint16_t*>(frag.dataBeginBytes() + sizeof(CAENV1730EventHeader));
    
    for(size_t i_ch=0; i_ch<nChannels; ++i_ch){
      fWvfmsVec[i_ch].resize(n_samples);
      
      //fill...
      for (size_t i_t=0; i_t<n_samples; ++i_t){
        if(i_t%2==0) fWvfmsVec[i_ch][i_t] = *(data+n_samples+i_t+1);
        else if(i_t%2==1) fWvfmsVec[i_ch][i_t] = *(data+n_samples+i_t-1);
      }
      //by here you have a vector<uint16_t> that is the waveform, in fWvfmsVec[i_ch]
      
      //get mean
      float wvfm_mean = std::accumulate(fWvfmsVec[i_ch].begin(),fWvfmsVec[i_ch].end(),0.0) / fWvfmsVec[i_ch].size();
      
      //get rms
      float wvfm_rms=0.0;
      for(auto const& val : fWvfmsVec[i_ch])
        wvfm_rms += (val-wvfm_mean)*(val-wvfm_mean);
      wvfm_rms = std::sqrt(wvfm_rms/fWvfmsVec[i_ch].size());
      
      nt_wvfm->Fill(eventNumber,header.eventCounter,header.triggerTimeTag,
                    i_ch,wvfm_mean,wvfm_rms,md->chTemps[i_ch]);
    }
  } */
  
}
DEFINE_ART_MODULE(sbndaq::CAENV1730WaveformAnalysis)
//this is where the name is specified
