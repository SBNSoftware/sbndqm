////////////////////////////////////////////////////////////////////////
// Class:       ExampleDatabaseStorage
// Module Type: analyzer
// File:        ExampleDatabaseStorage_module.cc
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

#include "sbndaq-redis-plugin/Utilities.h"
#include "sbndqm/DatabaseStorage/Histogram.h"

namespace sbndqm {
  class ExampleDatabaseStorage;
}

class sbndqm::ExampleDatabaseStorage : public art::EDAnalyzer {
  public:
    explicit ExampleDatabaseStorage(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
    virtual ~ExampleDatabaseStorage();
    virtual void analyze(art::Event const & evt);

  private:
    void SendHistogram();
    redisContext *fRedis;
    std::string fHistogramKey;
    unsigned fSleepTime;
};

sbndqm::ExampleDatabaseStorage::ExampleDatabaseStorage(fhicl::ParameterSet const & pset)
    : EDAnalyzer(pset),
      fSleepTime(pset.get<unsigned>("SleepTime", 0))  {
  // Initialize the redis connection
  fRedis = sbndaq::Connect2Redis(pset.get<std::string>("RedisServer", "localhost"), pset.get<int>("RedisPort", 6379), pset.get<std::string>("RedisPassword", ""));

  // get the key for this histogram
  fHistogramKey = pset.get<std::string>("HistogramKey");

}

sbndqm::ExampleDatabaseStorage::~ExampleDatabaseStorage() {}


void sbndqm::ExampleDatabaseStorage::analyze(art::Event const & evt)
{
  SendHistogram();
  if (fSleepTime > 0) {
    std::cout << "sleeping... " << std::endl;
    // sleep for a bit to simulate time between triggers
    sleep(fSleepTime);
  }
}


void sbndqm::ExampleDatabaseStorage::SendHistogram() {
  // setup an example Gaussian histogram
  TH1F *h = new TH1F("gaus", "gaus", 100, -5, 5);
  h->FillRandom("gaus", 10000);

  // draw it
  sbndqm::SendHistogram(fRedis, fHistogramKey, h, 10, 10, 300, 300);

  delete h;
}

DEFINE_ART_MODULE(sbndqm::ExampleDatabaseStorage)
