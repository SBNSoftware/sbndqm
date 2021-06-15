////////////////////////////////////////////////////////////////////////
/// \file   icaruscode/Decode/ChannelMapping/ICARUSChannelMapProvider.h
/// \author T. Usher (factorised by Gianluca Petrillo, petrillo@slac.stanford.edu)
/// \see    icaruscode/Decode/ChannelMapping/ICARUSChannelMapProvider.cxx
////////////////////////////////////////////////////////////////////////

#ifndef ICARUSCODE_DECODE_CHANNELMAPPING_ICARUSCHANNELMAPPROVIDER_H
#define ICARUSCODE_DECODE_CHANNELMAPPING_ICARUSCHANNELMAPPROVIDER_H

// ICARUS libraries
#include "sbndqm/dqmAnalysis/ChannelMapping/IICARUSChannelMap.h"
#include "sbndqm/dqmAnalysis/ChannelMapping/IChannelMapping.h"

// framework libraries
#include "fhiclcpp/ParameterSet.h"
#include "cetlib_except/exception.h"

// C/C++ standard libraries
#include <string>
#include <memory> // std::unique_ptr<>


// -----------------------------------------------------------------------------
namespace icarusDB { class ICARUSChannelMapProvider; }
class icarusDB::ICARUSChannelMapProvider: public IICARUSChannelMap
{
public:
    
    // Constructor, destructor.
    ICARUSChannelMapProvider(const fhicl::ParameterSet& pset);
    
    // Section to access fragment to board mapping
    bool                                    hasFragmentID(const unsigned int)       const override;
    const std::string&                      getCrateName(const unsigned int)        const override;
    const ReadoutIDVec&                     getReadoutBoardVec(const unsigned int)  const override;

    // Section to access channel information for a given board
    bool                                    hasBoardID(const unsigned int)          const override;
    unsigned int                            getBoardSlot(const unsigned int)        const override;
    const ChannelPlanePairVec&              getChannelPlanePair(const unsigned int) const override;

    // Section for PMT channel mapping
    bool                                    hasPMTDigitizerID(const unsigned int)   const override;
    const DigitizerChannelChannelIDPairVec& getChannelIDPairVec(const unsigned int) const override;
    
private:
    
    bool fDiagnosticOutput;
      
    IChannelMapping::TPCFragmentIDToReadoutIDMap   fFragmentToReadoutMap;
      
    IChannelMapping::TPCReadoutBoardToChannelMap   fReadoutBoardToChannelMap;

    IChannelMapping::FragmentToDigitizerChannelMap fFragmentToDigitizerMap; 

    std::unique_ptr<IChannelMapping>               fChannelMappingTool;

}; // icarusDB::ICARUSChannelMapProvider


// -----------------------------------------------------------------------------

#endif // ICARUSCODE_DECODE_CHANNELMAPPING_ICARUSCHANNELMAPPROVIDER_H
