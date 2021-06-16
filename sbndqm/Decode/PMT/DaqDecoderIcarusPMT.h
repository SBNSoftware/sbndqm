#ifndef SBNDQM_DECODE_PMT_DAQDECODERICARUSPMT_h
#define SBNDQM_DECODE_PMT_DAQDECODERICARUSPMT_h

////////////////////////////////////////////////////////////////////////
// Class:       DaqDecoderIcarus
// Plugin Type: producer (art v2_09_06)
// File:        DaqDecoderIcarus.h
//
// mailto: ascarpel@bnl.gov
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "lardataobj/RawData/RawDigit.h"
#include "artdaq-core/Data/Fragment.hh"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/TableAs.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/OptionalAtom.h"
#include "fhiclcpp/types/Atom.h"
#include "sbndaq-artdaq-core/Overlays/ICARUS/PhysCrateFragment.hh"
#include "canvas/Utilities/InputTag.h"

#include "canvas/Persistency/Common/FindMany.h"
#include "canvas/Persistency/Common/FindOne.h"

#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"

#include "sbndqm/dqmAnalysis/ChannelMapping/IICARUSChannelMap.h"

#include "lardataobj/RawData/OpDetWaveform.h"
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

namespace daq 
{

  class DaqDecoderIcarusPMT : public art::EDProducer 
  {

    public:

      struct Config 
      {

        using Name = fhicl::Name;

        using Comment = fhicl::Comment;

        fhicl::Sequence<art::InputTag> FragmentsLabels {
          Name("FragmentLabels"), 
          Comment("data products lables for the PMT fragments from the DAQ"), 
          std::vector<art::InputTag>{ "daq:CAENV1730", "daq:ContainerCAENV1730" }
        };

      };

      using Parameters = art::EDProducer::Table<Config>;

      explicit DaqDecoderIcarusPMT(Parameters const & params);

      DaqDecoderIcarusPMT(DaqDecoderIcarusPMT const &) = delete;
      
      DaqDecoderIcarusPMT(DaqDecoderIcarusPMT &&) = delete;
      
      DaqDecoderIcarusPMT & operator = (DaqDecoderIcarusPMT const &) = delete;
      
      DaqDecoderIcarusPMT & operator = (DaqDecoderIcarusPMT &&) = delete;

      std::vector<art::Handle<artdaq::Fragments>> readHandles( art::Event & event );

      artdaq::Fragments readFragments( std::vector<art::Handle<artdaq::Fragments>> handles );

      void processFragment( const artdaq::Fragment &artdaqFragment );

      void produce(art::Event & e) override;

    private:

      template <std::size_t NBits, typename T>
        static constexpr std::pair<std::array<std::size_t, NBits>, std::size_t>
          setBitIndices(T value) noexcept;
      
      std::vector<art::InputTag> m_input_tags;

      using OpDetWaveformCollection    = std::vector<raw::OpDetWaveform>;
      using OpDetWaveformCollectionPtr = std::unique_ptr<OpDetWaveformCollection>;
      OpDetWaveformCollectionPtr fOpDetWaveformCollection;  
    
      using PMTDigitizerInfoCollection    = std::vector<pmtAnalysis::PMTDigitizerInfo>;
      using PMTDigitizerInfoCollectionPtr = std::unique_ptr<PMTDigitizerInfoCollection>;
      PMTDigitizerInfoCollectionPtr fPMTDigitizerInfoCollection;  

      // Association

  };

}

#endif /* DaqDecoderIcarus_h */
