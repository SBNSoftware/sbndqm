#include <vector>

#include "TROOT.h"
#include "TTree.h"
#include "TH1.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art_root_io/TFileService.h"

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Analysis.hh"

/*
 * Uses the Analysis class to print stuff to file
*/

namespace tpcAnalysis {
  class TPCWavefromCreator;
}


class tpcAnalysis::TPCWavefromCreator : public art::EDAnalyzer {
public:
  explicit TPCWavefromCreator(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  TPCWavefromCreator(TPCWavefromCreator const &) = delete;
  TPCWavefromCreator(TPCWavefromCreator &&) = delete;
  TPCWavefromCreator & operator = (TPCWavefromCreator const &) = delete;
  TPCWavefromCreator & operator = (TPCWavefromCreator &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
private:
  tpcAnalysis::Analysis _analysis;
  TTree *_output;
  TH1F *_sample_hist;
  

};

tpcAnalysis::TPCWavefromCreator::TPCWavefromCreator(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p
{
  /*
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
  */

}

void tpcAnalysis::TPCWavefromCreator::analyze(art::Event const & e) {

  art::ServiceHandle<art::TFileService> fs;
  _sample_hist = fs->make<TH1F>("sample_hist","Histogram",1000,0,1000);
  _sample_hist->Fill(3);
  _sample_hist->Fill(20);


  //get the raw digits from the event
  art::Handle<std::vector<raw::RawDigit>> raw_digits_handle;
  e.getByLabel("daq", raw_digits_handle);

  //wes prefers working with vectors, but you can use the handle/pointer if you want
  std::vector<raw::RawDigit> const& raw_digits_vector(*raw_digits_handle);

  std::cout << "Size of vector is " << raw_digits_vector.size() << " or " << raw_digits_handle->size() << std::endl;

  //C-style loop
  for(size_t i_d=0; i_d<raw_digits_vector.size(); ++i_d){

    //using 'auto' here because I'm too lazy to type raw::RawDigit
    auto const& rd = raw_digits_vector[i_d];

    std::cout << "Looking at " << i_d << " rawdigit. Channel number is ... " << rd.Channel() << std::endl;
  }

  //python-style lopp
  for( auto const& rd : raw_digits_vector){
    std::cout << "...Channel number is ... " << rd.Channel() << std::endl;
  }


  //fin
}


DEFINE_ART_MODULE(tpcAnalysis::TPCWavefromCreator)
