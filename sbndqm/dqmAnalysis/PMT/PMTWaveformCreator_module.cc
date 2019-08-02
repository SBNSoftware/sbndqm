#include <vector>
#include "TROOT.h"
#include "TTree.h"
#include "TH1.h"
#include "TStopwatch.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art_root_io/TFileService.h"
//#include "ChannelData.hh"
#include "HeaderData.hh"
#include "art/Framework/Core/ModuleMacros.h"

#include "artdaq-core/Data/Fragment.hh"

#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "sbndaq-artdaq-core/Overlays/ICARUS/PhysCrateFragment.hh"
#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
#include "artdaq-core/Data/Fragment.hh"


//#include "Analysis.hh"
/*
 * Uses the Analysis class to print stuff to file
*/
namespace pmtAnalysis {
  class PMTWaveformCreator;
}
class pmtAnalysis::PMTWaveformCreator : public art::EDAnalyzer {
public:
  explicit PMTWaveformCreator(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.
  // Plugins should not be copied or assigned.
  PMTWaveformCreator(PMTWaveformCreator const &) = delete;
  PMTWaveformCreator(PMTWaveformCreator &&) = delete;
  PMTWaveformCreator & operator = (PMTWaveformCreator const &) = delete;
  PMTWaveformCreator & operator = (PMTWaveformCreator &&) = delete;
  // Required functions.
  void analyze(art::Event const & e) override;
private:
 // pmtAnalysis::Analysis _analysis;
  TTree *_output;
  TH1D *Times;
//  double stringTime = 0.0;
//  double makeStrings(raw::OpDetWaveform const&);
//  TStopwatch timer;
//  TStopwatch master;
};
pmtAnalysis::PMTWaveformCreator::PMTWaveformCreator(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p)
{
   art::ServiceHandle<art::TFileService> tfs; 
  Times = tfs->make<TH1D>("Times","Channel_Times",100,0,2);   

}
void pmtAnalysis::PMTWaveformCreator::analyze(art::Event const & evt) {
 // master.Start();
 // art::EventNumber_t eventNumber = evt.id().event();

  //get the OpDetWaveform from the event
  art::Handle<std::vector<raw::OpDetWaveform>>Op_Det_handle;
  evt.getByLabel("daq", Op_Det_handle);
 
  std::vector<raw::OpDetWaveform> const& Op_Det_vector(*Op_Det_handle);
  std::cout << "Size of vector is " << Op_Det_vector.size() << " or " << Op_Det_handle->size() << std::endl;
  
  for(size_t i_d=0; i_d<Op_Det_vector.size(); ++i_d){
    
    auto const& rd = Op_Det_vector[i_d];
    //std::cout << "Looking at " << i_d  << "rd is"<< < rd < std::endl;
 //   stringTime = makeStrings(rd);
 //   Times->Fill(stringTime);
  }
 
}
DEFINE_ART_MODULE(pmtAnalysis::PMTWaveformCreator)
