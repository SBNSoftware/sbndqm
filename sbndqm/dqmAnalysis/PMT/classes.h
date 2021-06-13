#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndqm/dqmAnalysis/PMT/PMTWaveform.hh"
#include "sbndqm/dqmAnalysis/PMT/PMTDigitizerInfo.hh"

namespace 
{
  
  struct dictionary 
  {

  	pmtAnalysis::PMTWaveform p;
  	std::vector<pmtAnalysis::PMTWaveform> p_v;
  	art::Wrapper<pmtAnalysis::PMTWaveform> p_w;
    art::Wrapper<std::vector<pmtAnalysis::PMTWaveform>> p_v_w;

    pmtAnalysis::PMTDigitizerInfo d;
    std::vector<pmtAnalysis::PMTDigitizerInfo> d_v;
    art::Wrapper<pmtAnalysis::PMTDigitizerInfo> d_w;
    art::Wrapper<std::vector<pmtAnalysis::PMTDigitizerInfo>> d_v_w;

  };

}