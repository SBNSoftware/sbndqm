#include <vector>

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art_root_io/TFileService.h"

#include "sbndqm/dqmAnalysis/TPC/ChannelDataSBND.hh"
#include "sbndqm/Decode/TPC/HeaderData.hh"
#include "sbndqm/dqmAnalysis/TPC/AnalysisSBND.hh"

#include "TROOT.h"
#include "TTree.h"

/*
 * Uses the Analysis class to print stuff to file
*/

namespace tpcAnalysis {
  class OfflineAnalysisSBND;
}


class tpcAnalysis::OfflineAnalysisSBND : public art::EDAnalyzer {
public:
  explicit OfflineAnalysisSBND(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OfflineAnalysisSBND(OfflineAnalysisSBND const &) = delete;
  OfflineAnalysisSBND(OfflineAnalysisSBND &&) = delete;
  OfflineAnalysisSBND & operator = (OfflineAnalysisSBND const &) = delete;
  OfflineAnalysisSBND & operator = (OfflineAnalysisSBND &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
private:
  tpcAnalysis::AnalysisSBND _analysis;
  TTree *_output;
};

tpcAnalysis::OfflineAnalysisSBND::OfflineAnalysisSBND(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p)
{
  art::ServiceHandle<art::TFileService> fs;
  _output = fs->make<TTree>("event", "event");
  // which data to use
  if (_analysis._config.reduce_data) {
    _output->Branch("channel_data", &_analysis._per_channel_data_reduced);
  }
  else {
    _output->Branch("channel_data", &_analysis._per_channel_data);
  }
  if (_analysis._config.n_headers > 0) {
    _output->Branch("header_data", &_analysis._header_data);
  }

}

void tpcAnalysis::OfflineAnalysisSBND::analyze(art::Event const & e) {
  _analysis.AnalyzeEvent(e);
  // calculate correlations here if you want to:
  // e.g. _analysis.Correlation(channel_i, channel_j);
  // or calculate the whole matrix:
  // _analysis.CorrelationMatrix()
  _output->Fill();
}


DEFINE_ART_MODULE(tpcAnalysis::OfflineAnalysisSBND)
