////////////////////////////////////////////////////////////////////////
// Class:       ExampleTimeStream
// Module Type: analyzer
// File:        ExampleTimeStream_module.cc
// Description: Saves information about each event to a MetricManager stream.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "../../MetricManagerShim/MetricManager.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <unistd.h>

namespace sbndqm {
  class ExampleTimeStream;
}

class sbndqm::ExampleTimeStream : public art::EDAnalyzer {

	public:
 	 explicit ExampleTimeStream(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  	 virtual ~ExampleTimeStream();

  	 virtual void analyze(art::Event const & evt);

	private:
};

sbndqm::ExampleTimeStream::ExampleTimeStream(fhicl::ParameterSet const & pset)
    : EDAnalyzer(pset)  {
  InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
}

sbndqm::ExampleTimeStream::~ExampleTimeStream() {}

void sbndqm::ExampleTimeStream::analyze(art::Event const & evt)
{
  double value = 5.0;
  int level = 3;
  const char *units = "units are ignored...";
  // send a test metric
  sendMetric("example_metric", value, units, level, artdaq::MetricMode::Average);

  std::cout << "sleeping... " << std::endl;
  // sleep for a second
  sleep(1);
}

DEFINE_ART_MODULE(sbndqm::ExampleTimeStream)
