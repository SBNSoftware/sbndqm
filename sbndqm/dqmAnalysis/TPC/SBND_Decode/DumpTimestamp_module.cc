#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <ctime>

#include "TROOT.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art/Framework/Services/Optional/TFileService.h"

#include "artdaq-core/Data/Fragment.hh"

/*
 * Uses the Analysis class to print stuff to file
*/

namespace daqAnalysis {
  class DumpTimestamp;
}


class daqAnalysis::DumpTimestamp : public art::EDAnalyzer {
public:
  explicit DumpTimestamp(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  DumpTimestamp(DumpTimestamp const &) = delete;
  DumpTimestamp(DumpTimestamp &&) = delete;
  DumpTimestamp & operator = (DumpTimestamp const &) = delete;
  DumpTimestamp & operator = (DumpTimestamp &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
private:
  art::InputTag _tag; 
};

daqAnalysis::DumpTimestamp::DumpTimestamp(fhicl::ParameterSet const & param):
  art::EDAnalyzer::EDAnalyzer(param),
  _tag(param.get<std::string>("raw_data_label", "daq"),param.get<std::string>("fragment_type_label", "NEVISTPC"))
{
}

void daqAnalysis::DumpTimestamp::analyze(art::Event const & event) {
  auto const& daq_handle = event.getValidHandle<artdaq::Fragments>(_tag);

  std::cout << "RUN: " << event.run() << " SUBRUN: " << event.subRun() << " EVENT: " << event.event() << std::endl;
  std::cout << "EVENT TIME " << event.time().timeLow() << std::endl;
  for (auto const &rawfrag: *daq_handle) { 
    std::cout << "TIMESTAMP: " << rawfrag.timestamp() << std::endl;
  } 
}


DEFINE_ART_MODULE(daqAnalysis::DumpTimestamp)
