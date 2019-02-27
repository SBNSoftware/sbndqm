#ifndef METRICMANAGER_SHIM_MM_HH
#define METRICMANAGER_SHIM_MM_HH

#include "artdaq-utilities/Plugins/MetricManager.hh"

// if metricMan is defined, it's included elsewhere -- we don't need to worry about it
#ifndef metricMan
artdaq::MetricManager *_I_am_the_metricMan = nullptr;
#endif

using namespace artdaq;

void InitializeMetricManager(fhicl::ParameterSet const& pset) {
  // don't re-initialize metricMan if it is defined elsewhere
  #ifndef metricMan
  assert(_I_am_the_metricMan == nullptr);
  std::cout << "Initializing Metric manager" << std::endl;
  _I_am_the_metricMan = new artdaq::MetricManager;
  _I_am_the_metricMan->initialize(pset);
  _I_am_the_metricMan->do_start();
  #else 
  std::cout << "Metric manager already initialized" << std::endl;
  #endif
}

#ifndef metricMan
#define metricMan _I_am_the_metricMan
#endif

// For send metrics with a raw name
void sendMetric(std::string const& name, std::string const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, int const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, double const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, float const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, long unsigned int const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, "", level, mode, metricPrefix, useNameOverride);
  }
}

std::string buildMetricName(std::string const& group, std::string const& instance, std::string const& metric) {
  return group + ":" + instance + ":" + metric;
}

void sendMetric(std::string const& group, std::string const& instance, std::string const& metric, 
    std::string const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(buildMetricName(group, instance, metric), value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& group, std::string const& instance, std::string const& metric, 
    int const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(buildMetricName(group, instance, metric), value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& group, std::string const& instance, std::string const& metric, 
    double const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(buildMetricName(group, instance, metric), value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& group, std::string const& instance, std::string const& metric, 
    float const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(buildMetricName(group, instance, metric), value, "", level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& group, std::string const& instance, std::string const& metric, 
    long unsigned int const& value, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(buildMetricName(group, instance, metric), value, "", level, mode, metricPrefix, useNameOverride);
  }
}

// For sending metrics through the config interface

#endif
