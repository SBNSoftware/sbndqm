////////////////////////////////////////////////////////////////////////
// Class:       PMTSunsetAnalyzer
// Plugin Type: analyzer (Unknown Unknown)
// File:        PMTSunsetAnalyzer_module.cc
//
// Generated at Thu Jun 13 16:50:34 2024 by Lynn Tung using cetskelgen
// from  version .
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/RawData/OpDetWaveform.h"
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"

namespace sunsetAna {
  class PMTSunsetAnalyzer;
}


class sunsetAna::PMTSunsetAnalyzer : public art::EDAnalyzer {
public:
  explicit PMTSunsetAnalyzer(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  PMTSunsetAnalyzer(PMTSunsetAnalyzer const&) = delete;
  PMTSunsetAnalyzer(PMTSunsetAnalyzer&&) = delete;
  PMTSunsetAnalyzer& operator=(PMTSunsetAnalyzer const&) = delete;
  PMTSunsetAnalyzer& operator=(PMTSunsetAnalyzer&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

private:

  float frms_threshold; 
  bool  fuse_local_baseline; 

  short estimateBaseline(std::vector<short> wvfm);
  // Declare member data here.

};


sunsetAna::PMTSunsetAnalyzer::PMTSunsetAnalyzer(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  frms_threshold = p.get<float>("rms_threshold",20); // threshold to determine whether or not the rms is above "normal" rms
  fuse_local_baseline = p.get<bool>("use_local_baseline",false); // whether or not to use the local baseline to determine the rms
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void sunsetAna::PMTSunsetAnalyzer::analyze(art::Event const& e)
{
  art::Handle opdetHandle = e.getHandle<std::vector<raw::OpDetWaveform>>("pmtdecoder");
  if( opdetHandle.isValid() && !opdetHandle->empty() ){
    std::vector<float> rms_v;
    for ( auto const & wvfm : *opdetHandle ) {
      // ch num = fragid*16 + digitizer channel;
      // ignore ch 15 
      if( wvfm.ChannelNumber()%16 == 15 ) continue;
      // ignore the timing caen (caen 8, or pmt09)
      if( wvfm.ChannelNumber()/16 > 7 ) continue;

      // get the baseline
      short baseline = estimateBaseline(wvfm);
      size_t rms_window = 10;
      float rms_maximum; 
      for (size_t i=0; i< wvfm.size() - rms_window; i++){
        float this_baseline = 0;
        if (fuse_local_baseline){
          for (size_t j=0; j<rms_window; j++)
            this_baseline += wvfm[i+j];
          this_baseline /= rms_window;
        }
        else this_baseline = baseline;
        float ret = 0;
        for (size_t j=0; j<rms_window; j++)
          ret += (wvfm[i+j] - this_baseline)*(wvfm[i+j] - this_baseline);
        float rms = sqrt(ret/rms_window);
        if (rms > rms_maximum) rms_maximum = rms;
      }
      if (rms_maximum > frms_threshold)
        rms_v.push_back(rms_maximum);

    }
    // get average of the rms vector 
    if (rms_v.size()){
      float rms_avg = 0;
      for (auto const & rms : rms_v)
        rms_avg += rms;
      rms_avg /= rms_v.size();
      std::cout << "average maximum rms: " << rms_avg << std::endl;
    }
  }
}

short sunsetAna::PMTSunsetAnalyzer::estimateBaseline(std::vector<short> wvfm){
  const auto median_it = wvfm.begin() + wvfm.size() / 2;
  std::nth_element(wvfm.begin(), median_it , wvfm.end());
  auto median = *median_it;
  return median;
}

DEFINE_ART_MODULE(sunsetAna::PMTSunsetAnalyzer)
