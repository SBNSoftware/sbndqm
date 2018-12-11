#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndcode/VSTAnalysis/ChannelData.hh"
#include "sbndcode/VSTAnalysis/HeaderData.hh"
#include "sbndcode/VSTAnalysis/NevisTPCMetaData.hh"

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

    tpcAnalysis::HeaderData h;
    std::vector<tpcAnalysis::HeaderData> h_v;
    art::Wrapper<tpcAnalysis::HeaderData> h_w;
    art::Wrapper<std::vector<tpcAnalysis::HeaderData>> h_v_w;

    tpcAnalysis::NevisTPCMetaData m;
    std::vector<tpcAnalysis::NevisTPCMetaData> m_v;
    art::Wrapper<tpcAnalysis::NevisTPCMetaData> m_w;
    art::Wrapper<std::vector<tpcAnalysis::NevisTPCMetaData>> m_v_w;

    std::vector<std::vector<short>> vs_v;
    art::Wrapper<std::vector<std::vector<short>>> vs_v_w;
    
  };
}


