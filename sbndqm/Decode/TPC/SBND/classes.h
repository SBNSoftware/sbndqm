#include "canvas/Persistency/Common/Wrapper.h"
#include <vector>
#include "sbndqm/Decode/TPC/SBND/DQMTPCDecodeAna.h"


namespace {
  struct dictionary {
    sbndqm::DQMTPCDecodeAna h;
    std::vector<sbndqm::DQMTPCDecodeAna> h_v;
    art::Wrapper<sbndqm::DQMTPCDecodeAna> h_w;
    art::Wrapper<std::vector<sbndqm::DQMTPCDecodeAna>> h_v_w;
  };
}


