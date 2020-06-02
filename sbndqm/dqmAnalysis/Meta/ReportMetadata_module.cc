#include <vector>
#include <chrono>

#include "TROOT.h"
#include "TTree.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 

#include "sbndaq-online/helpers/EventMeta.h"

/*
 * Uses the Analysis class to print stuff to file
*/

namespace sbndqm {
  class ReportMetadata;
}


class sbndqm::ReportMetadata : public art::EDAnalyzer {
public:
  explicit ReportMetadata(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  ReportMetadata(ReportMetadata const &) = delete;
  ReportMetadata(ReportMetadata &&) = delete;
  ReportMetadata & operator = (ReportMetadata const &) = delete;
  ReportMetadata & operator = (ReportMetadata &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
};

sbndqm::ReportMetadata::ReportMetadata(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p)
{
}

void sbndqm::ReportMetadata::analyze(art::Event const & e) {
  sbndaq::SendEventMeta("eventmeta", e);
}

DEFINE_ART_MODULE(sbndqm::ReportMetadata)
