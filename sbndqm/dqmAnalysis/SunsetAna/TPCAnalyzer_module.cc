////////////////////////////////////////////////////////////////////////
// Class:       TPCAnalyzer
// Plugin Type: analyzer (Unknown Unknown)
// File:        TPCAnalyzer_module.cc
//
// Generated at Thu Jun 13 16:50:27 2024 by Lynn Tung using cetskelgen
// from  version .
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace sunsetAna {
  class TPCAnalyzer;
}


class sunsetAna::TPCAnalyzer : public art::EDAnalyzer {
public:
  explicit TPCAnalyzer(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  TPCAnalyzer(TPCAnalyzer const&) = delete;
  TPCAnalyzer(TPCAnalyzer&&) = delete;
  TPCAnalyzer& operator=(TPCAnalyzer const&) = delete;
  TPCAnalyzer& operator=(TPCAnalyzer&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

private:

  // Declare member data here.

};


sunsetAna::TPCAnalyzer::TPCAnalyzer(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void sunsetAna::TPCAnalyzer::analyze(art::Event const& e)
{
  // Implementation of required member function here.
}

DEFINE_ART_MODULE(sunsetAna::TPCAnalyzer)
