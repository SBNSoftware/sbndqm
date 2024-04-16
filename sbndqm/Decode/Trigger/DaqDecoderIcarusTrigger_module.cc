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
#include <array>
#include <optional>
#include <iomanip>
#include <iostream>

#include "artdaq-core/Data/Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerV3Fragment.hh"
#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerInfo.hh"
#include "sbndqm/Decode/Trigger/detail/KeyedCSVparser.h"
#include "sbndqm/Decode/Trigger/detail/TriggerGateTypes.h"
#include "lardataobj/RawData/ExternalTrigger.h"
#include "lardataobj/RawData/TriggerData.h" // raw::Trigger
#include "lardataobj/Simulation/BeamGateInfo.h"
#include "sbnobj/Common/Trigger/ExtraTriggerInfo.h"
#include "sbnobj/Common/Trigger/BeamBits.h"

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

      // trigger bits extravaganza
      sbn::ExtraTriggerInfo::CryostatInfo unpackPrimitiveBits(
        std::size_t cryostat, bool firstEvent, unsigned long int counts,
        std::uint64_t connectors01, std::uint64_t connectors23,
        sbn::bits::triggerLogicMask triggerLogic
        ) const;

      std::array<std::uint64_t, sbn::ExtraTriggerInfo::MaxWalls> encodeLVDSbits(
        short int cryostat,
        std::uint64_t connector01word, std::uint64_t connector23word
      ) const;
    
      std::array<std::uint16_t, sbn::ExtraTriggerInfo::MaxWalls> encodeSectorBits(
        short int cryostat,
        std::uint64_t connector01word, std::uint64_t connector23word
      ) const;

      static std::uint64_t encodeLVDSbitsLegacy
        (short int cryostat, short int connector, std::uint64_t connectorWord);
    
      static std::uint16_t encodeSectorBitsLegacy
        (short int cryostat, short int connector, std::uint64_t connectorWord);

      // main method
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
 
      using ExtraInfoPtr = std::unique_ptr<sbn::ExtraTriggerInfo>;
      ExtraInfoPtr fTriggerExtra;

      double fTriggerTime; //us
      double fBNBgateDuration;
      double fNuMIgateDuration;
     
      /// Returns the `nBits` bits of `value` from `startBit` on.
      template <unsigned int startBit, unsigned int nBits, typename T>
        static constexpr T bits(T value);
 
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
  produces<sbn::ExtraTriggerInfo>();
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

sbn::ExtraTriggerInfo::CryostatInfo daq::DaqDecoderIcarusTrigger::unpackPrimitiveBits(
    std::size_t cryostat, bool firstEvent, unsigned long int counts,
    std::uint64_t connectors01, std::uint64_t connectors23,
    sbn::bits::triggerLogicMask triggerLogic
  ) const {
    sbn::ExtraTriggerInfo::CryostatInfo cryoInfo;
    
    // there is (or was?) a bug on the first event in the run,
    // which would make this triggerCount wrong
    cryoInfo.triggerCount = firstEvent? 0UL: counts,
    
    cryoInfo.LVDSstatus = encodeLVDSbits(cryostat, connectors01, connectors23);
    
    cryoInfo.sectorStatus
      = encodeSectorBits(cryostat, connectors01, connectors23);
    
    cryoInfo.triggerLogicBits = static_cast<unsigned int>(triggerLogic);
    
    return cryoInfo;
  } // TriggerDecoderV3::unpackPrimitiveBits()

// -----------------------------------------------------------------------------------------

std::array<std::uint64_t, sbn::ExtraTriggerInfo::MaxWalls>
  daq::DaqDecoderIcarusTrigger::encodeLVDSbits(
    short int cryostat,
    std::uint64_t connector01word, std::uint64_t connector23word
  ) const {
    
      return {
        encodeLVDSbitsLegacy
          (cryostat, sbn::ExtraTriggerInfo::EastPMTwall, connector01word),
        encodeLVDSbitsLegacy
          (cryostat, sbn::ExtraTriggerInfo::WestPMTwall, connector23word)
        }; // if no channel mapping
}

std::uint64_t daq::DaqDecoderIcarusTrigger::encodeLVDSbitsLegacy
    (short int cryostat, short int connector, std::uint64_t connectorWord)
  {
    /*
     * Encoding of the LVDS channels from the trigger:
     * * east wall:  `00<C0P2><C0P1><C0P0>00<C1P2><C1P1><C1P0>`
     * * west wall:  `00<C2P2><C2P1><C2P0>00<C3P2><C3P1><C3P0>`
     * The prescription from `sbn::ExtraTriggerInfo` translates into:
     * * east wall:  `00<C3P2><C3P1><C3P0>00<C2P2><C2P1><C2P0>`
     * * west wall:  `00<C1P2><C1P1><C1P0>00<C0P2><C0P1><C0P0>`
     * Therefore, the two 32-bit half-words need to be swapped
     * This holds for both cryostats, and both walls.
     * (note that the `00` bits may actually contain information from adders)
     */

    std::uint64_t lsw = connectorWord & 0x00FFFFFFULL;
    std::uint64_t msw = (connectorWord >> 32ULL) & 0x00FFFFFFULL;
    assert((connectorWord & 0x00FF'FFFF'00FF'FFFF) == ((msw << 32ULL) | lsw));
    std::swap(lsw, msw);
    return (msw << 32ULL) | lsw;
  } // TriggerDecoderV3::encodeLVDSbitsLegacy()

// -----------------------------------------------------------------------------------------

std::array<std::uint16_t, sbn::ExtraTriggerInfo::MaxWalls>
  daq::DaqDecoderIcarusTrigger::encodeSectorBits(
    short int cryostat,
    std::uint64_t connector01word, std::uint64_t connector23word
  ) const {
    
      return {
        encodeSectorBitsLegacy(cryostat, 2, connector23word),
        encodeSectorBitsLegacy(cryostat, 0, connector01word)
        };
} // no channel mapping
    
template <unsigned int startBit, unsigned int nBits, typename T>
  constexpr T daq::DaqDecoderIcarusTrigger::bits(T value) {
    constexpr T nBitsMask = (nBits == sizeof(T)*8)? ~T{0}: T((1 << nBits) - 1);
    return (value >> T(startBit)) & nBitsMask;
  }

std::uint16_t daq::DaqDecoderIcarusTrigger::encodeSectorBitsLegacy
    (short int cryostat, short int connector, std::uint64_t connectorWord)
  {
    /*
     * Encoding of the LVDS channels from the trigger (cf. `encodeLVDSbits()`):
     *  * connector 0: east wall south, LSB the south-most one
     *  * connector 1: east wall north, LSB the south-most one
     *  * connector 2: west wall south, LSB the south-most one
     *  * connector 3: west wall north, LSB the south-most one
     * Target:
     *  * 00000000 00sssnnn (both east and wall)
     */
    
    // connector (32 bit): 00000SSS LVDSLVDS LVDSLVDS LVDSLVDS
    constexpr std::size_t BitsPerConnector = 32;
    constexpr std::size_t FirstSectorBit = 27;
    constexpr std::size_t SectorBitsPerConnector = 3;
    
    return static_cast<std::uint16_t>(
        (bits<FirstSectorBit, SectorBitsPerConnector>(connectorWord) << SectorBitsPerConnector)
      | (bits<BitsPerConnector+FirstSectorBit, SectorBitsPerConnector>(connectorWord))
      );
  } // TriggerDecoderV3::encodeSectorBitsLegacy()

// -----------------------------------------------------------------------------------------

void daq::DaqDecoderIcarusTrigger::processFragment( const artdaq::Fragment &artdaqFragment ) {

  uint64_t const artdaq_ts = artdaqFragment.timestamp();
  icarus::ICARUSTriggerV3Fragment frag { artdaqFragment };
  
  std::string data = frag.GetDataString();
  icarus::ICARUSTriggerInfo datastream_info = icarus::parse_ICARUSTriggerV3String(data.data());
  auto const parsedData = parseTriggerStringAsCSV(data); 
  
  //std::cout << parsedData << std::endl;

  // TIMESTAMP BUSINESS
  static std::uint64_t raw_wr_ts =  makeTimestamp(frag.getWRSeconds(),frag.getWRNanoSeconds());

  // beam gate timestamp  
  std::uint64_t beamgate_ts { artdaq_ts };
  if (auto pBeamGateInfo = parsedData.findItem("Beam_TS")) {
    uint64_t const raw_bg_ts = makeTimestamp(pBeamGateInfo->getNumber<unsigned int>(1U),
                                             pBeamGateInfo->getNumber<unsigned int>(2U));
    beamgate_ts += raw_bg_ts - raw_wr_ts; //assumes no veto 
  }
  
  // enable gate timestamp
  std::uint64_t enablegate_ts { artdaq_ts };
  if (auto pEnableGateInfo = parsedData.findItem("Enable_TS")) {
    uint64_t const raw_en_ts = makeTimestamp(pEnableGateInfo->getNumber<unsigned int>(1U),
                                             pEnableGateInfo->getNumber<unsigned int>(2U));
    enablegate_ts += raw_en_ts - raw_wr_ts;
  }

  // no idea why this is useful...
  int64_t const WRtimeToTriggerTime = static_cast<int64_t>(artdaq_ts) - raw_wr_ts;

  unsigned int const triggerID = datastream_info.wr_event_no;
  int gate_type = datastream_info.gate_type;

  //fill raw::ExternalTrigger collection
  fTrigger->emplace_back(triggerID, artdaq_ts);

  //fill raw::Trigger collection 
  double gateStartFromTrigger = static_cast<double>(timestampDiff(beamgate_ts, artdaq_ts));
  double relGateStart =  gateStartFromTrigger/1000. - fTriggerTime; ///us
  fRelTrigger->emplace_back(triggerID, fTriggerTime, relGateStart, gate_type);
 
  //fill sim::BeamGateInfo collection
  auto beam_type = simGateType(gate_type);
  double gate_width = fBNBgateDuration;
  if( beam_type == sim::kNuMI ) gate_width = fNuMIgateDuration;
  fBeamGateInfo->emplace_back(beamgate_ts,gate_width,beam_type);

  //fill sbn::ExtraTriggerInfo
  sbn::triggerSource beamGateBit;
    
  switch (datastream_info.gate_type) {
      case TriggerGateTypes::BNB:{
        beamGateBit = sbn::triggerSource::BNB;
      if(datastream_info.trigger_type == 0)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesBNBMaj();
        fTriggerExtra->previousTriggerTimestamp = frag.getLastTimestampBNBMaj();
        fTriggerExtra->gateCount = datastream_info.gate_id_BNB;
        fTriggerExtra->triggerCount = frag.getTotalTriggerBNBMaj();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerBNBMaj();
      }
      else if(datastream_info.trigger_type == 1)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesBNBMinbias();
          fTriggerExtra->previousTriggerTimestamp = frag.getLastTimestampBNBMinbias();
          fTriggerExtra->gateCount = datastream_info.gate_id_BNB;
          fTriggerExtra->triggerCount = frag.getTotalTriggerBNBMinbias();
          fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerBNBMinbias();
      }
        break;
      }
      case TriggerGateTypes::NuMI:{
        beamGateBit = sbn::triggerSource::NuMI;
      if(datastream_info.trigger_type == 0)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesNuMIMaj();
        fTriggerExtra->previousTriggerTimestamp = frag.getLastTimestampNuMIMaj();
        fTriggerExtra->gateCount = datastream_info.gate_id_NuMI;
        fTriggerExtra->triggerCount = frag.getTotalTriggerNuMIMaj();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerNuMIMaj();
      }
      else if(datastream_info.trigger_type == 1)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesNuMIMinbias();
        fTriggerExtra->previousTriggerTimestamp = frag.getLastTimestampNuMIMinbias();
        fTriggerExtra->gateCount = datastream_info.gate_id_NuMI;
        fTriggerExtra->triggerCount = frag.getTotalTriggerNuMIMinbias();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerNuMIMinbias();
      }
        break;
      }
      case TriggerGateTypes::OffbeamBNB:{
        beamGateBit = sbn::triggerSource::OffbeamBNB;
      if(datastream_info.trigger_type == 0)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesBNBOffMaj();
        fTriggerExtra->previousTriggerTimestamp= frag.getLastTimestampBNBOffMaj();
        fTriggerExtra->gateCount = datastream_info.gate_id_BNBOff;
        fTriggerExtra->triggerCount = frag.getTotalTriggerBNBOffMaj();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerBNBOffMaj();
      }
      else if(datastream_info.trigger_type == 1)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesBNBOffMinbias();
        fTriggerExtra->previousTriggerTimestamp= frag.getLastTimestampBNBOffMinbias();
        fTriggerExtra->gateCount = datastream_info.gate_id_BNBOff;
        fTriggerExtra->triggerCount = frag.getTotalTriggerBNBOffMinbias();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerBNBOffMinbias();
      }
        break;
      }
      case TriggerGateTypes::OffbeamNuMI:{
        beamGateBit = sbn::triggerSource::OffbeamNuMI;
      if(datastream_info.trigger_type == 0)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesNuMIOffMaj();
        fTriggerExtra->previousTriggerTimestamp= frag.getLastTimestampNuMIOffMaj();
        fTriggerExtra->gateCount = datastream_info.gate_id_NuMIOff;
        fTriggerExtra->triggerCount = frag.getTotalTriggerNuMIOffMaj();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerNuMIOffMaj();
      }
      if(datastream_info.trigger_type == 1)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesNuMIOffMinbias();
        fTriggerExtra->previousTriggerTimestamp= frag.getLastTimestampNuMIOffMinbias();
        fTriggerExtra->gateCount = datastream_info.gate_id_NuMIOff;
        fTriggerExtra->triggerCount = frag.getTotalTriggerNuMIOffMinbias();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerNuMIOffMinbias();
      }
        break;
      }
      case TriggerGateTypes::Calib:{
        beamGateBit = sbn::triggerSource::Calib;
      if(datastream_info.trigger_type == 0)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesCalibMaj();
        fTriggerExtra->previousTriggerTimestamp = frag.getLastTimestampCalibMaj();
        //fTriggerExtra->gateCount = datastream_info.gate_id_calib;
        fTriggerExtra->triggerCount = frag.getTotalTriggerCalibMaj();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerCalibMaj();
      }
      if(datastream_info.trigger_type == 1)
      {
        fTriggerExtra->gateCountFromPreviousTrigger = frag.getDeltaGatesCalibMinbias();
        fTriggerExtra->previousTriggerTimestamp = frag.getLastTimestampCalibMinbias();
        //fTriggerExtra->gateCount = datastream_info.gate_id_calib;
        fTriggerExtra->triggerCount = frag.getTotalTriggerCalibMinbias();
        fTriggerExtra->anyTriggerCountFromPreviousTrigger = triggerID - frag.getLastTriggerCalibMinbias();
      }
      break;
      }
      default:                            beamGateBit = sbn::triggerSource::Unknown;
    } // switch gate type
		
    fTriggerExtra->sourceType = beamGateBit;
    fTriggerExtra->triggerType = static_cast<sbn::triggerType>(datastream_info.trigger_type);
    fTriggerExtra->triggerTimestamp = artdaq_ts;
    fTriggerExtra->beamGateTimestamp = beamgate_ts;
    fTriggerExtra->enableGateTimestamp = enablegate_ts;
    fTriggerExtra->triggerID = triggerID; //all triggers (event ID)
    fTriggerExtra->gateID = datastream_info.gate_id; //all gate types (gate ID)
    fTriggerExtra->anyGateCountFromAnyPreviousTrigger = frag.getDeltaGates();
    fTriggerExtra->anyPreviousTriggerTimestamp = frag.getLastTimestamp();
    
    sbn::triggerSource previousTriggerSourceBit;
    if(frag.getLastTriggerType() == 1)
      previousTriggerSourceBit = sbn::triggerSource::BNB;
    else if(frag.getLastTriggerType() == 2)
      previousTriggerSourceBit = sbn::triggerSource::NuMI;
    else if(frag.getLastTriggerType() == 3)
      previousTriggerSourceBit = sbn::triggerSource::OffbeamBNB;
    else if(frag.getLastTriggerType() == 4)
      previousTriggerSourceBit = sbn::triggerSource::OffbeamNuMI;
    else if(frag.getLastTriggerType() == 5)
      previousTriggerSourceBit = sbn::triggerSource::Calib;
    else
      previousTriggerSourceBit = sbn::triggerSource::Unknown;
    
    fTriggerExtra->anyPreviousTriggerSourceType = previousTriggerSourceBit;

    fTriggerExtra->WRtimeToTriggerTime = WRtimeToTriggerTime;
    sbn::bits::triggerLocationMask locationMask;
    // trigger location: 0x01=EAST; 0x02=WEST; 0x07=ALL
    int const triggerLocation = parsedData.getItem("Trigger Source").getNumber<int>(0);
    if(triggerLocation == 1)
      locationMask = mask(sbn::triggerLocation::CryoEast);
    else if(triggerLocation == 2)
      locationMask = mask(sbn::triggerLocation::CryoWest);
    else if(triggerLocation >= 3) // should be 7
      locationMask = mask(sbn::triggerLocation::CryoEast, sbn::triggerLocation::CryoWest);
    fTriggerExtra->triggerLocationBits = locationMask;
    
    //
    // fill sbn::ExtraTriggerInfo::cryostats
    //
    auto setCryoInfo = [
      this,&extra=*fTriggerExtra,isFirstEvent=(triggerID <= 1),data=parsedData
      ]
        (std::size_t cryo)
      {
        std::string const Side
          = (cryo == sbn::ExtraTriggerInfo::EastCryostat) ? "EAST": "WEST";
        std::string const CrSide = (cryo == sbn::ExtraTriggerInfo::EastCryostat)
          ? "Cryo1 EAST": "Cryo2 WEST";
        // trigger logic: 0x01=adders; 0x02=majority; 0x07=both
        std::string const triggerLogicKey = "MJ_Adder Source " + Side;
        int const triggerLogicCode = data.hasItem(triggerLogicKey)
          ? data.getItem(triggerLogicKey).getNumber<int>(0): 0;
        sbn::bits::triggerLogicMask triggerLogicMask;
        if(triggerLogicCode == 1)
          triggerLogicMask = mask(sbn::triggerLogic::PMTAnalogSum);
        else if(triggerLogicCode == 2)
          triggerLogicMask = mask(sbn::triggerLogic::PMTPairMajority);
        else if(triggerLogicCode >= 3) // should be 7
          triggerLogicMask = mask(sbn::triggerLogic::PMTAnalogSum, sbn::triggerLogic::PMTPairMajority);
        
        extra.cryostats[cryo] = unpackPrimitiveBits(
          cryo, isFirstEvent,
          data.getItem(CrSide + " counts").getNumber<unsigned long int>(0),
          data.getItem(CrSide + " Connector 0 and 1").getNumber<std::uint64_t>(0, 16),
          data.getItem(CrSide + " Connector 2 and 3").getNumber<std::uint64_t>(0, 16),
          triggerLogicMask
          );
      };
    
    if (triggerLocation & 1) setCryoInfo(sbn::ExtraTriggerInfo::EastCryostat);
    if (triggerLocation & 2) setCryoInfo(sbn::ExtraTriggerInfo::WestCryostat);


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
    fTriggerExtra = std::make_unique<sbn::ExtraTriggerInfo>();
    
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
    event.put(std::move(fTriggerExtra));
  }

  catch( cet::exception const& e ){
    mf::LogError("DaqDecoderIcarusTrigger") << "Error while attempting to decode trigger data:\n" << e.what();
    
    // fill empty products
    fTrigger = std::make_unique<TriggerCollection>();
    fRelTrigger = std::make_unique<RelativeTriggerCollection>();
    fBeamGateInfo = std::make_unique<BeamGateInfoCollection>();
    fTriggerExtra = std::make_unique<sbn::ExtraTriggerInfo>();
    event.put(std::move(fTrigger));
    event.put(std::move(fRelTrigger));
    event.put(std::move(fBeamGateInfo));
    event.put(std::move(fTriggerExtra));
  }

}

DEFINE_ART_MODULE(daq::DaqDecoderIcarusTrigger)
