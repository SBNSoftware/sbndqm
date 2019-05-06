////////////////////////////////////////////////////////////////////////
// Class:       SnapshotFilter
// Plugin Type: filter (art v2_07_03)
// File:        SnapshotFilter_module.cc
//
// Generated at Thu Jun 14 16:47:48 2018 by Gray Putnam using cetskelgen
// from cetlib version v3_00_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>

namespace daqAnalysis {
  class SnapshotFilter;
}


// Filter intended to be used with an Online Monitoring instance
// with "snapshots" enabled.
//
// Returns true on the nth event of each subrun (0 indedex), where n is set
// in the fhicl configuration by `event_delay` (0 by default)
class daqAnalysis::SnapshotFilter : public art::EDFilter {
public:
  explicit SnapshotFilter(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  SnapshotFilter(SnapshotFilter const &) = delete;
  SnapshotFilter(SnapshotFilter &&) = delete;
  SnapshotFilter & operator = (SnapshotFilter const &) = delete;
  SnapshotFilter & operator = (SnapshotFilter &&) = delete;

  // Required functions.
  bool filter(art::Event & e) override;

private:
  int _last_subrun;
  unsigned _event_delay;
  unsigned _event_number;
};


daqAnalysis::SnapshotFilter::SnapshotFilter(fhicl::ParameterSet const & p):
  art::EDFilter{p},
  _last_subrun(-1),
  _event_delay(p.get<unsigned>("event_delay", 0)),
  _event_number(0)
{}

bool daqAnalysis::SnapshotFilter::filter(art::Event & e)
{
  int this_subrun = (int) e.subRun();
  if (this_subrun != _last_subrun) {
    _event_number = 0;
  }
  else {
    _event_number ++;
  }
  _last_subrun = this_subrun;

  bool run = _event_number == _event_delay;
  if (run) {
    mf::LogDebug("SNAPSHOT FILTER") << "Snapshot filter running on subrun: " << this_subrun << " local event no: " << _event_number << " art event no: " << e.event() << std::endl;
  }
  return run;
}

DEFINE_ART_MODULE(daqAnalysis::SnapshotFilter)
