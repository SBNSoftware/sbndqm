#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndqm/Decode/TPC/SBND/TPCDecodeAna.h"


namespace {
  struct dictionary {
    sbndqm::TPCDecodeAna h;
    std::vector<sbndqm::TPCDecodeAna> h_v;
    art::Wrapper<sbndqm::TPCDecodeAna> h_w;
    art::Wrapper<std::vector<sbndqm::TPCDecodeAna>> h_v_w;
  };
}


