////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderIcarusTrigger
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderIcarusTrigger_module.cc
// Author:      M. Vicenzi (mvicenzi@bnl.gov)
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/InputTag.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib_except/exception.h"
#include "canvas/Utilities/InputTag.h"

#include <memory>
#include <vector>
#include <array>
#include <utility> // std::pair, std::move()
#include <stdlib.h>
#include <cassert>

#include "art/Framework/Core/ModuleMacros.h"
#include "artdaq-core/Data/Fragment.hh"
//#include "lardataobj/RawData/ExternalTrigger.h"
//#include "lardataobj/RawData/TriggerData.h" // raw::Trigger
//#include "lardataobj/Simulation/BeamGateInfo.h"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerV3Fragment.hh"

namespace daq 
{

  class DaqDecoderIcarusTrigger : public art::EDProducer 
  {

    public:
      
      explicit DaqDecoderIcarusTrigger(fhicl::ParameterSet const & pset);
      DaqDecoderIcarusTrigger(DaqDecoderIcarusTrigger const &) = delete;
      DaqDecoderIcarusTrigger(DaqDecoderIcarusTrigger &&) = delete; 
      DaqDecoderIcarusTrigger & operator = (DaqDecoderIcarusTrigger const &) = delete;
      DaqDecoderIcarusTrigger & operator = (DaqDecoderIcarusTrigger &&) = delete;

      // read & process fragments
      void processFragment( const artdaq::Fragment &artdaqFragment );

      void produce(art::Event & e) override;

    private:

      art::InputTag fFragmentsLabel;

      // define output structure?
  };

}

// -----------------------------------------------------------------------------------------

daq::DaqDecoderIcarusTrigger::DaqDecoderIcarusTrigger(fhicl::ParameterSet const & pset)
  : art::EDProducer(pset)
  , fFragmentsLabel{ pset.get<art::InputTag>("FragmentsLabel", "daq:ICARUSTriggerV3")  }
{
  
  // Output data products
//  produces<std::vector<raw::OpDetWaveform>>();
//  produces<std::vector<pmtAnalysis::PMTDigitizerInfo>>();

}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusTrigger::processFragment( const artdaq::Fragment &artdaqFragment ) {

  size_t const fragment_id = artdaqFragment.fragmentID();
  std::cout << fragment_id << std::endl;

  icarus::ICARUSTriggerV3Fragment frag { fragment };
  std::string data = frag.GetDataString();

  std::cout << data << std::endl;
  static std::uint64_t ts =  frag.getWRSeconds() * 1000000000ULL + frag.getWRNanoSeconds();
  std::cout << ts << std::endl; 

}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusTrigger::produce(art::Event & event) {

  try {
    // initialize the data product 
    //fOpDetWaveformCollection = std::make_unique<OpDetWaveformCollection>();
    //fPMTDigitizerInfoCollection = std::make_unique<PMTDigitizerInfoCollection>();

    // get fragments
    auto const & fragments = event.getValidHandle<artdaq::Fragments>(fFragmentsLabel);
    
    if( fragments.isValid() && fragments->size() > 0) {
      for (auto const & rawFrag: *fragments) 
        processFragment(rawFrag);
    }
    else
       mf::LogError("DaqDecoderIcarusTrigger")  << "No valid trigger fragments found";

    // Place the data product in the event stream
    // event.put(std::move(fOpDetWaveformCollection));
    // event.put(std::move(fPMTDigitizerInfoCollection));

  }

  catch( cet::exception const& e ){
    mf::LogError("DaqDecoderIcarusTrigger") << "Error while attempting to decode trigger data:\n" << e.what();
  }

}

DEFINE_ART_MODULE(daq::DaqDecoderIcarusTrigger)
