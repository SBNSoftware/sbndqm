////////////////////////////////////////////////////////////////////////
// Class:       NevisProductionTest
// Plugin Type: analyzer (art v3_04_00)
// File:        NevisProductionTest_module.cc
//
// Generated at Tue Sep  1 09:38:04 2020 by Gray Putnam using cetskelgen
// from cetlib version v3_09_00.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "artdaq-core/Data/Fragment.hh"

#include "sbndaq-artdaq-core/Overlays/SBND/NevisTPCFragment.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/NevisTPC/NevisTPCTypes.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/NevisTPC/NevisTPCUtilities.hh"

namespace dqm {
  class NevisProductionTest;
}


class dqm::NevisProductionTest : public art::EDAnalyzer {
public:
  explicit NevisProductionTest(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  NevisProductionTest(NevisProductionTest const&) = delete;
  NevisProductionTest(NevisProductionTest&&) = delete;
  NevisProductionTest& operator=(NevisProductionTest const&) = delete;
  NevisProductionTest& operator=(NevisProductionTest&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

private:

  // Declare member data here.

};


dqm::NevisProductionTest::NevisProductionTest(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void dqm::NevisProductionTest::analyze(art::Event const& e)
{
  art::InputTag tag {"daq", "NEVISTPC"};
  auto const& daq_handle = e.getValidHandle<artdaq::Fragments>(tag);

  for (unsigned i = 0; i < daq_handle->size(); i++) {
    const artdaq::Fragment &frag = daq_handle->at(i);
    // convert fragment to Nevis fragment
    sbndaq::NevisTPCFragment fragment(frag);

    std::unordered_map<uint16_t,sbndaq::NevisTPC_Data_t> waveform_map;
    size_t n_waveforms = fragment.decode_data(waveform_map);

    std::cout << "Number of waveforms in fragment: " << n_waveforms << std::endl;
  }
}

DEFINE_ART_MODULE(dqm::NevisProductionTest)
