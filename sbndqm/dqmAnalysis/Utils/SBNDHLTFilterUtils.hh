#ifndef SBNDQM_SBNDHLTFILTERUTILS_H_
#define SBNDQM_SBNDHLTFILTERUTILS_H_

// framework
#include "canvas/Persistency/Common/Ptr.h" 

//artdaq
#include "sbndaq-artdaq-core/Overlays/SBND/PTBFragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"

//c++
#include <vector>

namespace sbndqm {
  namespace SBNDHLTFilterUtils{
	
  std::vector<uint64_t> GetAllHLTs(artdaq::ContainerFragment *trigfrag);
  std::vector<uint64_t> GetHLT(sbndaq::CTBFragment ptb_fragment);
  bool ApplyGateFilter(std::vector<uint64_t> triggers, std::vector<uint64_t> ftrigger_type, std::vector<uint64_t> fexcluded_trigger);

  } //namespace SBNDHLTFilterUtils
} //namespace sbndqm

#endif
