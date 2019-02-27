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
#include "../../MetricConfig/ConfigureRedis.hh"

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
          void SendSingleMetric();
          void SendGroupMetrics();

          unsigned _sleep_time;
          double _value;
};

sbndqm::ExampleTimeStream::ExampleTimeStream(fhicl::ParameterSet const & pset)
    : EDAnalyzer(pset),
      _sleep_time(pset.get<unsigned>("sleep_time", 0))  {

  // Intiailize the metric manager
  sbndqm::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));

  // Initialize the config
  sbndqm::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));
}

sbndqm::ExampleTimeStream::~ExampleTimeStream() {}


void sbndqm::ExampleTimeStream::analyze(art::Event const & evt)
{
  SendSingleMetric();
  SendGroupMetrics();
  if (_sleep_time > 0) {
    std::cout << "sleeping... " << std::endl;
    // sleep for a bit to simulate time between triggers
    sleep(_sleep_time);
  }
}

void sbndqm::ExampleTimeStream::SendSingleMetric() {
  // level of importance of metric
  int level = 3;
  // The mode to accumulate the metrics. Here, the manager will report the average of 
  // of all metrics per reporting interval
  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  // The name of the stream in redis. In redis, this metric will be stored as a Stream
  // object with a key of: redis_name + redis_key_postfix, where redis_key_postfix is 
  // as configured in the fhicl file
  const char *name = "example_metric";
  // value of metric
  double value = 5.;
  // send a metric 
  sbndqm::sendMetric(name, value, level, mode);
}

void sbndqm::ExampleTimeStream::SendGroupMetrics() {
  // send metrics for a number of "wires"

  // level and aggregation mode
  int level = 3;
  artdaq::MetricMode mode = artdaq::MetricMode::Average;

  // name of the metric -- should match the name we assigned in metric_config
  const char *metric_name = "rms";
  // name of the group -- should match the name we assigned in metric_config
  const char *group_name = "example";
  
  // make metrics for 10 "wires"
  for (unsigned i = 0; i < 10; i++) {
    // name of this instance
    std::string instance = std::to_string(i);
    // metric value
    double value = (double) i;
    sbndqm::sendMetric(group_name, instance, metric_name, value, level, mode); 
  }
}


DEFINE_ART_MODULE(sbndqm::ExampleTimeStream)
