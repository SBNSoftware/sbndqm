////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderIcarusTrigger
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderIcarusTrigger_module.cc
// Author:      M. Vicenzi (mvicenzi@bnl.gov)
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
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
#include <cstdlib>
#include <string_view>

#include "artdaq-core/Data/Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerV3Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerInfo.hh"
#include "sbndqm/Decode/Trigger/detail/KeyedCSVparser.h"
#include "lardataobj/RawData/ExternalTrigger.h"
#include "lardataobj/RawData/TriggerData.h" // raw::Trigger
#include "lardataobj/Simulation/BeamGateInfo.h"

namespace daq 
{
  using namespace std::string_literals;
  
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
      
      // parsing trigger data
      static std::string_view firstLine(std::string const& s, std::string const& endl = "\0\n\r"s);
      icarus::details::KeyValuesData parseTriggerStringAsCSV(std::string const& data) const;

      void produce(art::Event & e) override;

    private:
     
      art::InputTag fFragmentsLabel;

      // define output data products
      using TriggerCollection = std::vector<raw::ExternalTrigger>;
      using TriggerPtr = std::unique_ptr<TriggerCollection>;
      TriggerPtr fTrigger;
    
      using RelativeTriggerCollection = std::vector<raw::Trigger>;
      using RelativeTriggerPtr = std::unique_ptr<RelativeTriggerCollection>;
      RelativeTriggerPtr fRelTrigger;
    
      double fTriggerTime; //us
  };
  
  struct TriggerGateTypes {
    static constexpr int BNB { 1 };
    static constexpr int NuMI { 2 };
    static constexpr int OffbeamBNB { 3 };
    static constexpr int OffbeamNuMI { 4 };
    static constexpr int Calib { 5 };
  };

}

// -----------------------------------------------------------------------------------------

daq::DaqDecoderIcarusTrigger::DaqDecoderIcarusTrigger(fhicl::ParameterSet const & pset)
  : art::EDProducer(pset)
  , fFragmentsLabel{ pset.get<art::InputTag>("FragmentsLabel", "daq:ICARUSTriggerV3")  }
  , fTriggerTime{ 1500. }
{
  
  // Output data products
  produces<TriggerCollection>();
  produces<RelativeTriggerCollection>();

}

// ----------------------------------------------------------------------------------------

std::string_view daq::DaqDecoderIcarusTrigger::firstLine(std::string const& s, std::string const& endl){
    return { s.data(), std::min(s.find_first_of(endl), s.size()) };
}

// ----------------------------------------------------------------------------------------

icarus::details::KeyValuesData daq::DaqDecoderIcarusTrigger::parseTriggerStringAsCSV(std::string const& data) const {
 
  icarus::details::KeyedCSVparser parser;
  parser.addPatterns({
        { "Cryo. (EAST|WEST) Connector . and .", 1U }
        , { "Trigger Type", 1U }
  });
  std::string_view const dataLine = firstLine(data);
  try {
    return parser(dataLine);
  }
  
  catch(icarus::details::KeyedCSVparser::Error const& e) {
    mf::LogError("DaqDecoderIcarusTrigger")
      << "Error parsing " << dataLine.length()
      << "-char long trigger string:\n==>|" << dataLine
      << "|<==\nError message: " << e.what() << std::endl;
    throw;
  }
} 

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusTrigger::processFragment( const artdaq::Fragment &artdaqFragment ) {

  //size_t const fragment_id = artdaqFragment.fragmentID(); 
  uint64_t const artdaq_ts = artdaqFragment.timestamp();

  icarus::ICARUSTriggerV3Fragment frag { artdaqFragment };
  static std::uint64_t raw_wr_ts =  frag.getWRSeconds() * 1000000000ULL + frag.getWRNanoSeconds();
  std::string data = frag.GetDataString();
  icarus::ICARUSTriggerInfo datastream_info = icarus::parse_ICARUSTriggerV3String(data.data());
  auto const parsedData = parseTriggerStringAsCSV(data); 

  unsigned int const triggerID = datastream_info.wr_event_no;
  int gate_type = datastream_info.gate_type;

  //fill raw::ExternalTrigger collection
  fTrigger->emplace_back(triggerID, artdaq_ts);

  //fill raw::Trigger collection 
  std::uint64_t beamgate_ts { artdaq_ts };
  if (auto pBeamGateInfo = parsedData.findItem("Beam_TS")) {
    uint64_t const raw_bg_ts = pBeamGateInfo->getNumber<unsigned int>(1U) * 1000000000ULL 
                             + pBeamGateInfo->getNumber<unsigned int>(2U);
    beamgate_ts += raw_bg_ts - raw_wr_ts; //assumes no veto
  }
  double gateStartFromTrigger = (long long int)beamgate_ts - (long long int)artdaq_ts; //ns
  double relGateStart = fTriggerTime + gateStartFromTrigger/1000.; ///us
  fRelTrigger->emplace_back(triggerID, fTriggerTime, relGateStart, gate_type);

}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusTrigger::produce(art::Event & event) {

  try {

    // get fragments
    auto const & fragments = event.getValidHandle<artdaq::Fragments>(fFragmentsLabel);
    
    // initialize data products
    fTrigger = std::make_unique<TriggerCollection>();
    fRelTrigger = std::make_unique<RelativeTriggerCollection>();
    
    if( fragments.isValid() && fragments->size() > 0) {
      for (auto const & rawFrag: *fragments) 
        processFragment(rawFrag);
    }
    else
       mf::LogError("DaqDecoderIcarusTrigger")  << "No valid trigger fragments found";

    // place data products in the event stream
    event.put(std::move(fTrigger));
    event.put(std::move(fRelTrigger));

  }

  catch( cet::exception const& e ){
    mf::LogError("DaqDecoderIcarusTrigger") << "Error while attempting to decode trigger data:\n" << e.what();
  }

}

DEFINE_ART_MODULE(daq::DaqDecoderIcarusTrigger)
