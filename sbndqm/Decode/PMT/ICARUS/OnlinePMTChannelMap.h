////////////////////////////////////////////////////////////////////////
// File:        OnlinePMTChannelMap.h
// Author:      M. Vicenzi (mvicenzi@bnl.gov)
// Description:
//  Online channel numbering of the ICARUS PMT digitizers, [0, 384):
//  - PMT channels (0 to 359) use the offline (LArSoft) channel ID from
//    the channel mapping database (`IICARUSChannelMap` service);
//  - the special (16th) channel of each digitizer, not in the database,
//    gets `specialChannelBase + board`, `board` being the effective
//    fragment ID (0 to 23): with the defaults, channels 360 to 383.
////////////////////////////////////////////////////////////////////////

#ifndef SBNDQM_DECODE_PMT_ICARUS_ONLINEPMTCHANNELMAP_H
#define SBNDQM_DECODE_PMT_ICARUS_ONLINEPMTCHANNELMAP_H

#include <array>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace pmtAnalysis
{

  class OnlinePMTChannelMap {

  public:

    static constexpr unsigned int NChannelsPerBoard = 16;
    static constexpr unsigned int NoChannel = std::numeric_limits<unsigned int>::max();

    OnlinePMTChannelMap( unsigned int nBoards = 24,
                         unsigned int specialChannelIndex = 15,
                         unsigned int specialChannelBase = 360 )
      : m_nBoards( nBoards )
      , m_specialChannelIndex( specialChannelIndex )
      , m_specialChannelBase( specialChannelBase )
    {};

    // Rebuilds the cache from the channel mapping (`icarusDB::IICARUSChannelMapProvider`,
    // templated so that modules using only the special channels do not need icaruscode);
    // call at each new run. Returns the list of problems found, for the caller to report.
    template <typename ChannelMap>
    std::vector<std::string> update( ChannelMap const& channelMap );

    // channel number of `digitizerChannel` on `board` (effective fragment ID), `NoChannel` if unknown
    unsigned int channel( unsigned int board, unsigned int digitizerChannel ) const {
      auto const it = m_channels.find(board);
      if( it == m_channels.end() || digitizerChannel >= NChannelsPerBoard ) return NoChannel;
      return it->second[digitizerChannel];
    };

    // the special channels do not need the database
    bool isSpecialChannel( unsigned int channel ) const
      { return channel >= m_specialChannelBase && channel < m_specialChannelBase + m_nBoards; };
    unsigned int specialChannelBoard( unsigned int channel ) const { return channel - m_specialChannelBase; };

    unsigned int nBoards() const { return m_nBoards; };
    unsigned int nChannels() const { return m_specialChannelBase + m_nBoards; };

  private:

    unsigned int m_nBoards;
    unsigned int m_specialChannelIndex;
    unsigned int m_specialChannelBase;

    std::map<unsigned int, std::array<unsigned int, NChannelsPerBoard>> m_channels;

  };

}

// -----------------------------------------------------------------------------------------

template <typename ChannelMap>
std::vector<std::string> pmtAnalysis::OnlinePMTChannelMap::update( ChannelMap const& channelMap ) {

  std::vector<std::string> problems;
  m_channels.clear();

  for( unsigned int board = 0; board < m_nBoards; board++ ) {

    std::string const board_s = "Board " + std::to_string(board);

    if( !channelMap.hasPMTDigitizerID(board) ) {
      problems.push_back( board_s + " not found in the channel mapping database: its channels will be skipped" );
      continue;
    }

    auto& channels = m_channels[board];
    channels.fill(NoChannel);

    for( auto const& info : channelMap.getPMTchannelInfo(board) ) {

      if( info.digitizerChannelNo >= NChannelsPerBoard || info.digitizerChannelNo == m_specialChannelIndex ) continue;
      if( info.channelID >= m_specialChannelBase ) {
        problems.push_back( board_s + " channel " + std::to_string(info.digitizerChannelNo)
                            + " has database channel ID " + std::to_string(info.channelID)
                            + ", not below the special channels (" + std::to_string(m_specialChannelBase) + "): skipped" );
        continue;
      }
      channels[info.digitizerChannelNo] = info.channelID;
    }

    channels.at(m_specialChannelIndex) = m_specialChannelBase + board;

    for( unsigned int ch = 0; ch < NChannelsPerBoard; ch++ ) {
      if( channels[ch] == NoChannel )
        problems.push_back( board_s + " channel " + std::to_string(ch)
                            + " not found in the channel mapping database: it will be skipped" );
    }
  }

  return problems;
}

#endif
