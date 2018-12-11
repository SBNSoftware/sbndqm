#ifndef VSTChannelMap_h
#define VSTChannelMap_h

#include <map>
#include <vector>

#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "fhiclcpp/ParameterSet.h"

#include "HeaderData.hh"
#include "NevisTPCMetaData.hh"

namespace tpcAnalysis {

struct ReadoutChannel {
    uint32_t crate;
    uint32_t slot;
    uint32_t channel_ind;
};

class VSTChannelMap {
public:
  explicit VSTChannelMap(fhicl::ParameterSet const & p, art::ActivityRegistry & areg);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  unsigned NFEM() const;
  unsigned NChannels() const;
  unsigned NCrates() const { return 1; /* 1 for VST */ }
  unsigned Channel2Wire(unsigned channel_no) const;
  unsigned Channel2Wire(unsigned channel_ind, unsigned slot_ind, unsigned crate_ind, bool add_offset=false) const;
  unsigned Channel2Wire(tpcAnalysis::ReadoutChannel channel) const;
  unsigned Wire2Channel(unsigned wire_no) const;
  tpcAnalysis::ReadoutChannel Ind2ReadoutChannel(unsigned channel_no) const;
  unsigned ReadoutChannel2Ind(tpcAnalysis::ReadoutChannel channel) const;
  unsigned ReadoutChannel2Ind(unsigned channel, unsigned slot, unsigned crate, bool add_offset=false) const;

  unsigned ReadoutChannel2FEMInd(tpcAnalysis::ReadoutChannel channel) const;
  unsigned ReadoutChannel2FEMInd(unsigned channel, unsigned slot, unsigned crate, bool add_offset=false) const;

  bool IsMappedChannel(unsigned channel_no) const;
  bool IsMappedChannel(unsigned channel_ind, unsigned slot_ind, unsigned crate_ind, bool add_offset=false) const;
  bool IsMappedChannel(tpcAnalysis::ReadoutChannel channel) const;

  // defined slot index for the two different ways that information is encoded
  unsigned SlotIndex(tpcAnalysis::HeaderData header) const;
  unsigned SlotIndex(tpcAnalysis::NevisTPCMetaData metadata) const;
  unsigned SlotIndex(tpcAnalysis::ReadoutChannel header) const;

  bool IsGoodSlot(unsigned slot) const;

  unsigned NSlotWire(unsigned slot) const;
  unsigned NSlotChannel() const;

  // Hard code this for online monitoring
  // 1 == induction plane
  // 2 == collection plane
  unsigned PlaneType(unsigned wire_no) const {
    bool is_induction = wire_no < 240;
    if (is_induction) {
      return 1;
    }
    else {
      return 2;
    }
  }

private:
  // Declare member data here.
  unsigned _n_channels;
  unsigned _n_fem;
  unsigned _channel_per_fem;
  unsigned _slot_offset;
  unsigned _crate_id;
  std::map<unsigned, unsigned> _channel_to_wire;
  std::map<unsigned, unsigned> _wire_to_channel;
  std::vector<unsigned> _wire_per_fem;
  std::vector<std::vector<unsigned>> _fem_active_channels;
};

}// end namespace

DECLARE_ART_SERVICE(tpcAnalysis::VSTChannelMap, LEGACY)

#endif

