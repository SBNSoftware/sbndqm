#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndqm/Decode/TPC/HeaderData.hh"


namespace {
  struct dictionary {
    tpcAnalysis::HeaderData h;
    std::vector<tpcAnalysis::HeaderData> h_v;
    art::Wrapper<tpcAnalysis::HeaderData> h_w;
    art::Wrapper<std::vector<tpcAnalysis::HeaderData>> h_v_w;
  };
}


