#ifndef METRICMANAGER_SHIM_MM_HH
#define METRICMANAGER_SHIM_MM_HH

#include "artdaq-utilities/Plugins/MetricManager.hh"

// if metricMan is defined, it's included elsewhere -- we don't need to worry about it
#ifndef metricMan
artdaq::MetricManager *_I_am_the_metricMan = nullptr;
#define metricMan _I_am_the_metricMan
#endif

using namespace artdaq;

void InitializeMetricManager(fhicl::ParameterSet const& pset) {
  // don't re-initialize metricMan if it is defined elsewhere
  #ifndef metricMan
  assert(metricMan == nullptr);
  metricMan->initialize(pset);
  #endif
}

void sendMetric(std::string const& name, std::string const& value, std::string const& unit, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, unit, level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, int const& value, std::string const& unit, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, unit, level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, double const& value, std::string const& unit, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, unit, level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, float const& value, std::string const& unit, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, unit, level, mode, metricPrefix, useNameOverride);
  }
}

void sendMetric(std::string const& name, long unsigned int const& value, std::string const& unit, int level, MetricMode mode, std::string const& metricPrefix = "", bool useNameOverride = false) {
  if (metricMan != NULL) {
    metricMan->sendMetric(name, value, unit, level, mode, metricPrefix, useNameOverride);
  }
}
#endif
