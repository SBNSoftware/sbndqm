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
          unsigned _sleep_time;
};

sbndqm::ExampleTimeStream::ExampleTimeStream(fhicl::ParameterSet const & pset)
    : EDAnalyzer(pset),
      _sleep_time(pset.get<unsigned>("sleep_time", 0))  {

  // Intiailize the metric manager
  InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));

}

sbndqm::ExampleTimeStream::~ExampleTimeStream() {}

void sbndqm::ExampleTimeStream::analyze(art::Event const & evt)
{
  // the value to send out at this instance
  double value = 5.0;
  // level of importance of metric
  int level = 3;
  // Metric Manager provides "units" field, but they are ignored by the redis plugin
  const char *units = "units are ignored...";
  // The mode to accumulate the metrics. Here, the manager will report the average of 
  // of all metrics per reporting interval
  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  // The name of the stream in redis. In redis, this metric will be stored as a Stream
  // object with a key of: redis_name + redis_key_postfix, where redis_key_postfix is 
  // as configured in the fhicl file
  const char *name = "example_metric";
  // send a metric 
  sendMetric(name, value, units, level, mode);

  if (_sleep_time > 0) {
    std::cout << "sleeping... " << std::endl;
    // sleep for a bit to simulate time between triggers
    sleep(_sleep_time);
  }
}

DEFINE_ART_MODULE(sbndqm::ExampleTimeStream)
