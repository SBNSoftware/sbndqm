#include "canvas/Persistency/Common/Wrapper.h"
#include "canvas/Persistency/Common/Assns.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTData.h"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTHit.h"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTTrack.h"
#include <ostream>
#include <vector>
#include <utility>

template class art::Wrapper<sbndqm::crt::CRTData>;
template class std::vector<sbndqm::crt::CRTData>;
template class art::Wrapper<std::vector<sbndqm::crt::CRTData> >;
