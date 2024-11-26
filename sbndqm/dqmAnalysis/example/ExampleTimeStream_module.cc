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

#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"

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
          void SendSingleInvertedMetric();
          void SendGroupMetrics();

          unsigned _sleep_time;
          double _value;
};

sbndqm::ExampleTimeStream::ExampleTimeStream(fhicl::ParameterSet const & pset)
    : EDAnalyzer(pset),
      _sleep_time(pset.get<unsigned>("sleep_time", 0))  {

  // Initialize the config
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));
}

sbndqm::ExampleTimeStream::~ExampleTimeStream() {}


void sbndqm::ExampleTimeStream::analyze(art::Event const & evt)
{
  SendSingleMetric();
  SendSingleInvertedMetric();
  SendGroupMetrics();
  if (_sleep_time > 0) {
    std::cout << "sleeping... " << std::endl;
    // sleep for a bit to simulate time between triggers
    sleep(_sleep_time);
  }
}

void sbndqm::ExampleTimeStream::SendSingleMetric() {
  // level of importance of metric
  int level = -1;
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
  sbndaq::sendMetric(name, value, level, mode);
}

// Same as before -- but this metric will be inverted after being stored in the database
void sbndqm::ExampleTimeStream::SendSingleInvertedMetric() {
  int level = -1;
  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  // The INVERT directive instructs the front end website to invert the metric for display.
  // During averaging and storage the metric will not be altered. 
  // Also -- the "INVERT/" substring will be stripped when storing in redis
  const char *name = "INVERT/example_metric_inverted";
  double value = 5.;
  sbndaq::sendMetric(name, value, level, mode);
}

void sbndqm::ExampleTimeStream::SendGroupMetrics() {
  // send metrics for a number of "wires"

  // level and aggregation mode
  int level = -1;
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
    sbndaq::sendMetric(group_name, instance, metric_name, value, level, mode); 
  }
}


DEFINE_ART_MODULE(sbndqm::ExampleTimeStream)
