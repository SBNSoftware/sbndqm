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
#include <utility> // std::pair, std::move()
#include <cstdlib>
#include <string_view>
#include <chrono>

#include "artdaq-core/Data/Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerV3Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerInfo.hh"
#include "sbndqm/Decode/Trigger/detail/KeyedCSVparser.h"
#include "sbndqm/Decode/Trigger/detail/TriggerGateTypes.h"
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
      icarus::KeyValuesData parseTriggerStringAsCSV(std::string const& data) const;
      static std::uint64_t makeTimestamp(unsigned int s, unsigned int ns) 
        { return s * 1000000000ULL + ns; }
      static long long int timestampDiff(std::uint64_t a, std::uint64_t b)
        { return static_cast<long long int>(a) - static_cast<long long int>(b); }
      sim::BeamType_t simGateType (int source);

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
      
      using BeamGateInfoCollection = std::vector<sim::BeamGateInfo>;
      using BeamGateInfoCollectionPtr = std::unique_ptr<BeamGateInfoCollection>;
      BeamGateInfoCollectionPtr fBeamGateInfo;
    
      double fTriggerTime; //us
      double fBNBgateDuration;
      double fNuMIgateDuration;

  };
 
}

// -----------------------------------------------------------------------------------------

daq::DaqDecoderIcarusTrigger::DaqDecoderIcarusTrigger(fhicl::ParameterSet const & pset)
  : art::EDProducer(pset)
  , fFragmentsLabel{ pset.get<art::InputTag>("FragmentsLabel", "daq:ICARUSTriggerV3")  }
  , fTriggerTime{ 1500. }
  , fBNBgateDuration{ 1.6 }
  , fNuMIgateDuration{ 9.5 }
{
  
  // Output data products
  produces<TriggerCollection>();
  produces<RelativeTriggerCollection>();
  produces<BeamGateInfoCollection>();
}

// ----------------------------------------------------------------------------------------

std::string_view daq::DaqDecoderIcarusTrigger::firstLine(std::string const& s, std::string const& endl){
    return { s.data(), std::min(s.find_first_of(endl), s.size()) };
}

// ----------------------------------------------------------------------------------------

icarus::KeyValuesData daq::DaqDecoderIcarusTrigger::parseTriggerStringAsCSV(std::string const& data) const {
 
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

sim::BeamType_t daq::DaqDecoderIcarusTrigger::simGateType(int source){
    
  switch (source) {
    case daq::TriggerGateTypes::BNB:
    case daq::TriggerGateTypes::OffbeamBNB:
      return sim::kBNB;
    case daq::TriggerGateTypes::NuMI:
    case daq::TriggerGateTypes::OffbeamNuMI:
      return sim::kNuMI;
    case daq::TriggerGateTypes::Calib:
      return sim::kUnknown;
    default:
      mf::LogWarning("DaqDecoderIcarusTrigger") << "Unsupported trigger gate type " << source;
      return sim::kUnknown;
  }
}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusTrigger::processFragment( const artdaq::Fragment &artdaqFragment ) {

  uint64_t const artdaq_ts = artdaqFragment.timestamp();
  icarus::ICARUSTriggerV3Fragment frag { artdaqFragment };
  
  std::uint64_t raw_wr_ts =  makeTimestamp(frag.getWRSeconds(),frag.getWRNanoSeconds());
  std::string data = frag.GetDataString();
  icarus::ICARUSTriggerInfo datastream_info = icarus::parse_ICARUSTriggerV3String(data.data());
  auto const parsedData = parseTriggerStringAsCSV(data); 
  
  //std::cout << parsedData << std::endl;

  unsigned int const triggerID = datastream_info.wr_event_no;
  int gate_type = datastream_info.gate_type;

  //fill raw::ExternalTrigger collection
  fTrigger->emplace_back(triggerID, artdaq_ts);

  //fill raw::Trigger collection 
  std::uint64_t beamgate_ts { artdaq_ts };
  if (auto pBeamGateInfo = parsedData.findItem("Beam_TS")) {
    uint64_t const raw_bg_ts = makeTimestamp(pBeamGateInfo->getNumber<unsigned int>(1U),
                                             pBeamGateInfo->getNumber<unsigned int>(2U));
    beamgate_ts += raw_bg_ts - raw_wr_ts; //assumes no veto 
  }
  double gateStartFromTrigger = static_cast<double>(timestampDiff(beamgate_ts, artdaq_ts));
  double relGateStart =  gateStartFromTrigger/1000. - fTriggerTime; ///us
  fRelTrigger->emplace_back(triggerID, fTriggerTime, relGateStart, gate_type);
 
  //fill sim::BeamGateInfo collection
  auto beam_type = simGateType(gate_type);
  double gate_width = fBNBgateDuration;
  if( beam_type == sim::kNuMI ) gate_width = fNuMIgateDuration;
  fBeamGateInfo->emplace_back(beamgate_ts,gate_width,beam_type);

}

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusTrigger::produce(art::Event & event) {

  try {

    // get fragments
    auto const & fragments = event.getValidHandle<artdaq::Fragments>(fFragmentsLabel);
    
    // initialize data products
    fTrigger = std::make_unique<TriggerCollection>();
    fRelTrigger = std::make_unique<RelativeTriggerCollection>();
    fBeamGateInfo = std::make_unique<BeamGateInfoCollection>();
    
    if( fragments.isValid() && fragments->size() > 0) {
      for (auto const & rawFrag: *fragments) 
        processFragment(rawFrag);
    }
    else
       mf::LogError("DaqDecoderIcarusTrigger")  << "No valid trigger fragments found";

    // place data products in the event stream
    event.put(std::move(fTrigger));
    event.put(std::move(fRelTrigger));
    event.put(std::move(fBeamGateInfo));
  }

  catch( cet::exception const& e ){
    mf::LogError("DaqDecoderIcarusTrigger") << "Error while attempting to decode trigger data:\n" << e.what();
    
    // fill empty products
    fTrigger = std::make_unique<TriggerCollection>();
    fRelTrigger = std::make_unique<RelativeTriggerCollection>();
    fBeamGateInfo = std::make_unique<BeamGateInfoCollection>();
    event.put(std::move(fTrigger));
    event.put(std::move(fRelTrigger));
    event.put(std::move(fBeamGateInfo));
  }

}

DEFINE_ART_MODULE(daq::DaqDecoderIcarusTrigger)
