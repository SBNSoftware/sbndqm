#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndqm/dqmAnalysis/TPC/ChannelData.hh"
#include "sbndqm/dqmAnalysis/TPC/ChannelDataSBND.hh"

namespace {
  struct dictionary {
    tpcAnalysis::ChannelData c;
    std::vector<tpcAnalysis::ChannelData> c_v;
    art::Wrapper<tpcAnalysis::ChannelData> c_w;
    art::Wrapper<std::vector<tpcAnalysis::ChannelData>> c_v_w;

    tpcAnalysis::ReducedChannelData rc;
    std::vector<tpcAnalysis::ReducedChannelData> rc_v;
    art::Wrapper<tpcAnalysis::ReducedChannelData> rc_w;
    art::Wrapper<std::vector<tpcAnalysis::ReducedChannelData>> rc_v_w;

    std::vector<std::vector<short>> vs_v;
    art::Wrapper<std::vector<std::vector<short>>> vs_v_w;
    
  };
}


