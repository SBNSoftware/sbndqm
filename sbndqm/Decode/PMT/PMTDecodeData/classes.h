#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"

namespace 
{
  
  struct dictionary 
  {

    pmtAnalysis::PMTDigitizerInfo d;
    std::vector<pmtAnalysis::PMTDigitizerInfo> d_v;
    art::Wrapper<pmtAnalysis::PMTDigitizerInfo> d_w;
    art::Wrapper<std::vector<pmtAnalysis::PMTDigitizerInfo>> d_v_w;
  };

}