////////////////////////////////////////////////////////////////////////
// Class:       ExampleAlarm
// Module Type: analyzer
// File:        ExampleAlarm_module.cc
// Description: Saves information about each event to a MetricManager stream.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <unistd.h>

#include "TH1F.h"

#include "sbndaq-online/helpers/Alarm.h"

namespace sbndqm {
  class ExampleAlarm;
}

class sbndqm::ExampleAlarm : public art::EDAnalyzer {
  public:
    explicit ExampleAlarm(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
    virtual ~ExampleAlarm();
    virtual void analyze(art::Event const & evt);

  private:
    std::string fAlarm;
    unsigned fSleepTime;
};

sbndqm::ExampleAlarm::ExampleAlarm(fhicl::ParameterSet const & pset)
    : EDAnalyzer(pset),
      fSleepTime(pset.get<unsigned>("SleepTime", 0))  {
  // get the key for this histogram
  fAlarm = pset.get<std::string>("Alarm", "This is an example alarm.");
}

sbndqm::ExampleAlarm::~ExampleAlarm() {}


void sbndqm::ExampleAlarm::analyze(art::Event const & evt)
{
  sbndaq::SendAlarm(fAlarm, evt, "This is a description of the alarm.");
}

DEFINE_ART_MODULE(sbndqm::ExampleAlarm)
