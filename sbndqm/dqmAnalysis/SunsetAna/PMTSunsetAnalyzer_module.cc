////////////////////////////////////////////////////////////////////////
// Class:       PMTSunsetAnalyzer
// Plugin Type: analyzer (Unknown Unknown)
// File:        PMTSunsetAnalyzer_module.cc
//
// Generated at Thu Jun 13 16:50:34 2024 by Lynn Tung using cetskelgen
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

#include "lardataobj/RawData/OpDetWaveform.h"
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"

namespace sunsetAna {
  class PMTSunsetAnalyzer;
}


class sunsetAna::PMTSunsetAnalyzer : public art::EDAnalyzer {
public:
  explicit PMTSunsetAnalyzer(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  PMTSunsetAnalyzer(PMTSunsetAnalyzer const&) = delete;
  PMTSunsetAnalyzer(PMTSunsetAnalyzer&&) = delete;
  PMTSunsetAnalyzer& operator=(PMTSunsetAnalyzer const&) = delete;
  PMTSunsetAnalyzer& operator=(PMTSunsetAnalyzer&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

private:

  // Declare member data here.

};


sunsetAna::PMTSunsetAnalyzer::PMTSunsetAnalyzer(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void sunsetAna::PMTSunsetAnalyzer::analyze(art::Event const& e)
{
  art::Handle opdetHandle = e.getHandle<std::vector<raw::OpDetWaveform>>("pmtdecoder");
  if( opdetHandle.isValid() && !opdetHandle->empty() ){
    std::cout << "# of waveforms: " << opdetHandle->size() << std::endl;
    for ( auto const & wvfm : *opdetHandle ) {
      std::cout<< "ch num: " << wvfm.ChannelNumber() << std::endl;
      // ch num = digitizer channel + nchannels per board * fragid
    }
  }
  else{
    std::cout << "no waveforms found!" << std::endl;
  }
}

DEFINE_ART_MODULE(sunsetAna::PMTSunsetAnalyzer)
