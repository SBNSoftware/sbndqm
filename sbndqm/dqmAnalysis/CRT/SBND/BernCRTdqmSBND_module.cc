////////////////////////////////////////////////////////////////////////
// Class:       BernCRTdqmSBND
// Module Type: analyzer
// File:        BernCRTdqmSBND_module.cc
// Description:
// Modified by Matt King January 2024 for use on SBND
// mking9@uchicago.edu
// Additionally modified by Nikki Pallat. Last update made on November 18 2024.
// palla110@umn.edu
// Significant refactor January 2025 by Henry Lay
// h.lay@sheffield.ac.uk
// Refactors modified by John Plows, Feb-Mar 2025.
// kplows@liverpool.ac.uk
//
// This Module sends metrics from the SBND CRT modules to the redis database
// 
/*
 * Current metrics being monitored:
 *       Board-level:
 *               MissingT0         - Total number of hits on this board in this event with missing T0 flag
 *               MissingT1         - Total number of hits on this board in this event with missing T1 flag
 *               ReadoutRate       - How many non-clock reset hits were there on this board in this event?
 *               T0ClockDrift      - For T0 reset events, difference of T0 timestamp from exactly 1e9ns (1s)
 *               Baseline          - Average pedestal across all channels on this board, with the exception of max and pair of max, and any channels that surpass fBigHitADCThreshold
 *               AverageADC        - Same as Baseline but reported as LastPoint in sbndaq::sendMetric
 *               Deadtime          - Time difference between consecutive hits of any type (minimum value should be deadtime)
 *               PullWindow        - Difference between first & last timestamp for that board in the event (maximum value should be the pull window)
 *               NT0Resets         - Number of T0 reset events in this board in this event
 *               NT1Resets         - Number of T1 reset events in this board in this event
 *               T1ResetTDCDiff    - Similar to T0ClockDrift, difference between the T0 timestamp of the T1 reset and the value recorded in the TDC
 *               MaxADCValue       - ADC value from highest-ADC channel
 *               MaxADCValuePair   - ADC value from pair of highest-ADC channel
 *               MaxADCChannel     - Index of channel with highest ADC
 *               MaxADCChannelPair - Index of channel paired to the one with highest ADC
 *               earlysynch        - Distance between last poll start and hit timestamp
 *               latesynch         - Distance between hit timestamp and end of this poll
 *
 *       Channel-level:
 *               ChReadoutRate  - How many non-clock reset hits were there on this board where this channel was the largest in this event?
 *               Pedestal       - Pedestal mean for a channel
 *               ADC            - Value of ADC when this channel is max (or paired with max)
 *
 *	Event-level:
 *             T0ResetSpread  - The range between the lowest & highest T0 values for T0 reset events seen across all boards
 *             T1ResetSpread  - The range between the lowest & highest T0 values for T1 reset events seen across all boards
 *
 *	Fragment-Level:
 *		frag_count - number of fragments sent 
 *		zero_rate - number of empty fragments sent
 */
//
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "sbndaq-artdaq-core/Overlays/Common/BernCRTFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/RawEvent.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/TDCTimestampFragment.hh"
#include "sbndaq-artdaq-core/Overlays/Common/BernCRTTranslator.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"

#include "sbndaq-artdaq-core/Overlays/Common/BernCRTTranslator.hh"

#include "TH1F.h"
#include "TNtuple.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <unistd.h>

namespace sbndaq {
  class BernCRTdqmSBND;
}

/*****/

class sbndaq::BernCRTdqmSBND : public art::EDAnalyzer {

public:
  explicit BernCRTdqmSBND(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~BernCRTdqmSBND();
  
  virtual void analyze(art::Event const & evt);
  uint64_t GetRawEventTime(art::Event const &evt);
  uint64_t GetSPECTDCT1ResetTime(art::Event const &evt, const uint64_t &rawEventTS);
  void reconfigure(fhicl::ParameterSet const & pset);
 
private:

  //fhicl parameters
  bool                     fDebug;
  std::string              fCRTModuleLabel;
  std::string              fCRTInstanceLabel;
  std::string              fSPECTDCModuleLabel;
  std::vector<std::string> fSPECTDCInstanceLabels;
  std::string              fDAQHeaderModuleLabel;
  std::string              fDAQHeaderInstanceLabel;
  uint16_t                 fSPECTDCT1Channel;
  uint64_t                 fRawTSCorrection;
  uint16_t                 fBigHitADCThreshold;
  uint16_t                 fBoardsRequiredForResetSpread;
  double                   fRateNormalisation;
  std::vector<uint8_t>     fMac5s;
  
  //fhicl parameters
  int fBeamWindowStart;
  int fBeamWindowEnd;
  
};

//Define the constructor
sbndaq::BernCRTdqmSBND::BernCRTdqmSBND(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_fragment_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_event_config"));

  this->reconfigure( pset );
}

sbndaq::BernCRTdqmSBND::~BernCRTdqmSBND()
{
}

void sbndaq::BernCRTdqmSBND::analyze(art::Event const & evt) {
  //sleep(2);

  if (fDebug) std::cout << "######################################################################" << std::endl;
  if (fDebug) std::cout << std::endl;  
  if (fDebug) std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();

  uint64_t rawEventTS = GetRawEventTime(evt);

  uint64_t tdcT1Reset = GetSPECTDCT1ResetTime(evt, rawEventTS);

  // Initialise per module and per channel vars
  std::map<uint8_t, int> hitCount;
  std::map<uint8_t, uint64_t> prevFragmentTS;
  std::map<uint8_t, uint16_t> readoutRate;
  std::map<uint8_t, uint16_t> missingT0;
  std::map<uint8_t, uint16_t> missingT1;
  std::map<uint8_t, uint64_t> minTS;
  std::map<uint8_t, uint64_t> maxTS;
  std::map<uint8_t, uint16_t> nT0Resets;
  std::map<uint8_t, uint16_t> nT1Resets;
  std::map<uint8_t, uint32_t> t0Reset;
  std::map<uint8_t, uint32_t> t1Reset;
  
  std::map<uint8_t, std::map<uint8_t, float>> chReadoutRate;

  for(const uint8_t& mac5 : fMac5s)
    {
      hitCount[mac5]       = 0;
      prevFragmentTS[mac5] = 0;
      readoutRate[mac5]    = 0;
      missingT0[mac5]      = 0;
      missingT1[mac5]      = 0;
      minTS[mac5]          = std::numeric_limits<uint64_t>::max();
      maxTS[mac5]          = 0;
      nT0Resets[mac5]      = 0;
      nT1Resets[mac5]      = 0;
      t0Reset[mac5]        = std::numeric_limits<uint32_t>::max();
      t1Reset[mac5]        = std::numeric_limits<uint32_t>::max();

      for(int ch = 0; ch < 32; ++ch)
        chReadoutRate[mac5][ch] = 0.;
    }

  std::vector<icarus::crt::BernCRTTranslator> hit_vector;
  
  if (fDebug) std::cout<<"Hit vector declared. Going to getMany fragments";
  
  std::string fCRTModuleLabel = "daq";
  std::string CRTInstanceLabel = "ContainerBERNCRTV2";

  art::Handle<std::vector<artdaq::Fragment>> fragmentHandle;
  evt.getByLabel(fCRTModuleLabel, CRTInstanceLabel, fragmentHandle);


  if (fDebug) std::cout<<"evt.getByLabel successful.";

  if(!fragmentHandle.isValid() || fragmentHandle->size() == 0)
    return;

  auto this_hit_vector = icarus::crt::BernCRTTranslator::getCRTData(*fragmentHandle);
  if (fDebug) std::cout<<"Successfully obtained CRT data" << std::endl;
    
  /////////////////////////////////
  // Send Fragment Level Metrics //
  /////////////////////////////////
    
  //Copied from FragmentDQMAna_module.cc
    
  for (auto const& frag : *fragmentHandle){
    //frag is artdaq::Fragment

    // if fragment is a container fragment, print # of fragments in that container fragment
    if(frag.type() != artdaq::Fragment::ContainerFragmentType) {
      if (fDebug) std::cout<<"Fragment type is incorrect!";
      continue;}

    artdaq::ContainerFragment cont_frag(frag);
    
    unsigned int const fragid = frag.fragmentID();
    std::string fragment_id = std::to_string(fragid);

    //get frag count
    uint64_t frag_count = cont_frag.block_count();
    //get zero rate
    uint64_t nzero = 0;
    if (frag_count == 0) { nzero = 1; }

    std::string group_name = "unknown_cont_frag";

    if (cont_frag.fragment_type() == sbndaq::detail::FragmentType::CAENV1730) {group_name = "PMT_cont_frag";}
    else if (cont_frag.fragment_type() == sbndaq::detail::FragmentType::BERNCRTV2) {group_name = "CRT_cont_frag";}
    if (fDebug) std::cout<<"fragment_id: "<<fragment_id<<std::endl;

    sbndaq::sendMetric(group_name, fragment_id, "frag_count", frag_count, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric(group_name, fragment_id, "zero_rate", nzero, 0, artdaq::MetricMode::Rate);
     
  }//end loop over handle

  //Concatenate hit vectors from each fragment into an event-long hit vector.
  hit_vector.insert(hit_vector.end(),this_hit_vector.begin(),this_hit_vector.end());
  
  ///////////////////////////////////////
  // Extract Information from the Hits //
  ///////////////////////////////////////
  
  //Variables used in Grafana to be sent to DQM OM:
  size_t num_t1_resets = 0;
  size_t hitsperplane[7] = {0,0,0,0,0,0,0};

  //loop over all CRT hits in an event
  for(const auto & hit : hit_vector) {

    // Extract core hit information
    const uint8_t& mac5 = hit.mac5;

    // Extract metadata information
    const uint64_t& fragmentTS = hit.timestamp;
 
    std::string mac5Str = std::to_string(mac5);
    if(fDebug) std::cout << "Mac5: " << mac5Str <<std::endl;

    const uint64_t & fragment_timestamp = hit.timestamp;

    //data from FEB:
    std::string mac5_str = std::to_string(mac5);

    //store the timing and flag information from a hit
    const uint32_t ts0        =  static_cast<uint32_t>(hit.ts0);
    const uint32_t ts1        =  static_cast<uint32_t>(hit.ts1);
    const bool     isTs0Reset =  hit.IsReference_TS0();
    const bool     isTs1Reset =  hit.IsReference_TS1();
    const bool     isTs0Good  = !hit.IsOverflow_TS0();
    const bool     isTs1Good  = !hit.IsOverflow_TS1();
    
    const uint16_t * adc = hit.adc;

    const uint64_t & this_poll_end             = hit.this_poll_end;
    const uint64_t & last_poll_start           = hit.last_poll_start;

    size_t maxadc        = 0; int maxindex = -1;
    size_t totaladc   = 0;
    size_t ADCchannel = 0;

    //let's fill our sample hist with the Time_TS0()-1e9 if 
    //it's a GPS reference pulse
    if(isTs0Reset){
      if (fDebug) std::cout<<" TS0 "<<ts0 - 1e9<<std::endl;
    }
    if(isTs1Reset){
      if (fDebug) std::cout<<" TS1 "<<ts1 - 1e9<<std::endl; 
      num_t1_resets++;
    }
    
    ///////////////////////////
    // Channel-Level Metrics //
    ///////////////////////////
  
    for(int i = 0; i<32; i++) {
      ADCchannel = adc[i];
      
      //Send Channel-Level Metrics to the database
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 100 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average); 
    }

    int pairindex = -1;
    // Get the max ADC and its pair for this hit - only send if not a reset
    if( !isTs0Reset && !isTs1Reset && isTs0Good ) {
      ++readoutRate[mac5];
      
      for( int ch = 0; ch < 32; ch++ ){
	if( adc[ch] > maxadc ) { maxadc = adc[ch]; maxindex = ch; }
      }
      ++chReadoutRate[mac5][maxindex];
      pairindex = ( maxindex % 2 == 1 ) ? maxindex - 1 : maxindex + 1;
      
      sbndaq::sendMetric("CRT_board", mac5_str, "MaxADCValue", maxadc, 0, artdaq::MetricMode::LastPoint);
      sbndaq::sendMetric("CRT_board", mac5_str, "MaxADCChannel", maxindex, 0, artdaq::MetricMode::LastPoint);
      sbndaq::sendMetric("CRT_board", mac5_str, "MaxADCValuePair", adc[pairindex], 0, artdaq::MetricMode::LastPoint);
      sbndaq::sendMetric("CRT_board", mac5_str, "MaxADCChannelPair", pairindex, 0, artdaq::MetricMode::LastPoint);
    }

    // We also want to keep track of the averaged ADC of each channel over time (Pedestal),
    // and the average of all ADC over boards at each point (AverageADC) (Board)

    if( (!isTs0Reset && !isTs1Reset && isTs0Good) || (isTs0Reset || isTs1Reset) ) {
      int nBaselineChannels = 0;
      for( int ch = 0; ch < 32; ch++ ) {
	if( ch == maxindex || ch == pairindex ) continue;
	if( adc[ch] > fBigHitADCThreshold ) continue;

	totaladc += adc[ch];
	nBaselineChannels++;

	std::string chStr = std::to_string(mac5*100 + ch);
	sbndaq::sendMetric("CRT_channel", chStr, "Pedestal", adc[ch], 0, artdaq::MetricMode::Average);
      }

      // guard against 0 channels contributing
      nBaselineChannels = std::max(nBaselineChannels, 1);
      int baseline = totaladc / nBaselineChannels;
      if( (!isTs0Reset && !isTs1Reset && isTs0Good) ) {
	sbndaq::sendMetric("CRT_board", mac5_str, "baseline", baseline, 0, artdaq::MetricMode::Average);
	sbndaq::sendMetric("CRT_board", mac5_str, "AverageADC", baseline, 0, artdaq::MetricMode::LastPoint);
      }
    }

    // Calculate deadtime
    if( hitCount[mac5] == 0 )
      prevFragmentTS[mac5] = fragmentTS;
    else {
      uint64_t diff = fragmentTS - prevFragmentTS[mac5];
      
      if(fDebug) std::cout << "Sending metric Deadtime with value " << diff << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "Deadtime", diff, 0, artdaq::MetricMode::Minimum);
      prevFragmentTS[mac5] = fragmentTS;
    }

    if(fragmentTS < minTS[mac5])
        minTS[mac5] = fragmentTS;

      if(fragmentTS > maxTS[mac5])
        maxTS[mac5] = fragmentTS;

    ++hitCount[mac5];

    uint64_t earlysynch = last_poll_start - fragment_timestamp;
    uint64_t latesynch = fragment_timestamp - this_poll_end;
    
    //From the code that writes to Grafana	
    auto thisone = hit.fragment_ID;  uint plane = (thisone & 0x0700) >> 8;
    
    if (fDebug) std::cout<<"Plane: "<<plane<<std::endl;
    
    if (plane>7) {if (fDebug) std::cout << "bad plane value " << plane << std::endl; plane=0;}
  
    auto thisflag = hit.flags;
   
    if(!isTs0Good)
      ++missingT0[mac5];
    
    if(!isTs1Good)
      ++missingT1[mac5];

    // require that this is data and not clock reset (0xC), and that the ts1 time is valid (0x2)
    if (thisflag & 0x2 && !(thisflag & 0xC) ) {
      // check ts1 for beam window
      if(fDebug) std::cout<<"It's a data event! Ts1: "<<ts1<<std::endl;
      if ((int)ts1>fBeamWindowStart && (int)ts1<fBeamWindowEnd) hitsperplane[plane]++;
    }

    if(isTs0Reset)
      {
	++nT0Resets[mac5];

	if(isTs0Good)
	  t0Reset[mac5] = ts0;
      }

    if(isTs1Reset)
      {
	++nT1Resets[mac5];
	
	if(isTs0Good)
	  {
	    if(t1Reset[mac5] != std::numeric_limits<uint32_t>::max())
	      {
		uint32_t fracTDCT1Reset = tdcT1Reset % static_cast<uint32_t>(1e9);
		uint32_t diff           = ts0 > fracTDCT1Reset ? ts0 - fracTDCT1Reset : fracTDCT1Reset - ts0;
		uint32_t currDiff       = t1Reset[mac5] > fracTDCT1Reset ? t1Reset[mac5] - fracTDCT1Reset : fracTDCT1Reset - t1Reset[mac5];
		
		if(diff < currDiff)
		  t1Reset[mac5] = ts0;
	      }
	    else
	      t1Reset[mac5] = ts0;
	  }
      }    

    // Documentation on sbndaq::sendMetric lives in MetricManager.hh

    sbndaq::sendMetric("CRT_board", mac5_str, "MissingT0", missingT0[mac5], 0, artdaq::MetricMode::Maximum);
    sbndaq::sendMetric("CRT_board", mac5_str, "MissingT1", missingT1[mac5], 0, artdaq::MetricMode::Maximum);
 
    //only send clockdrift info when it makes sense to do so; that is, for T0 reset events.
    if(isTs0Reset && isTs0Good) {sbndaq::sendMetric("CRT_board", mac5_str, "T0clockdrift", static_cast<int>(ts0) - 1e9, 0, artdaq::MetricMode::LastPoint);}

    //Sychronization Metrics
    sbndaq::sendMetric("CRT_board", mac5_str, "earlysynch", earlysynch, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric("CRT_board", mac5_str, "latesynch", latesynch, 0, artdaq::MetricMode::Average);

  } //loop over all CRT hits in an event

  uint32_t t0ResetMin = std::numeric_limits<uint32_t>::max();
  uint32_t t0ResetMax = std::numeric_limits<uint32_t>::lowest();
  uint32_t t1ResetMin = std::numeric_limits<uint32_t>::max();
  uint32_t t1ResetMax = std::numeric_limits<uint32_t>::lowest();

  uint16_t boardsWithT0Reset = 0;
  uint16_t boardsWithT1Reset = 0;
 
  for(const uint8_t& mac5 : fMac5s)
    {
      std::string mac5Str = std::to_string(mac5);

      if(fDebug) std::cout << "Sending metric ReadoutRate with value " << readoutRate[mac5]  / fRateNormalisation << std::endl;
      sbndaq::sendMetric("CRT_board", mac5Str, "ReadoutRate", readoutRate[mac5] / fRateNormalisation, 0, artdaq::MetricMode::Average);

      if(fDebug) std::cout << "Sending metric PullWindow with value " << maxTS[mac5] - minTS[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5Str, "PullWindow", maxTS[mac5] - minTS[mac5], 0, artdaq::MetricMode::Maximum);

      if(fDebug) std::cout << "Sending metric NT0Resets with value " << nT0Resets[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5Str, "NT0Resets", nT0Resets[mac5], 0, artdaq::MetricMode::Maximum);

      if(fDebug) std::cout << "Sending metric NT1Resets with value " << nT1Resets[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5Str, "NT1Resets", nT1Resets[mac5], 0, artdaq::MetricMode::Maximum);

      if(nT0Resets[mac5] == 1 && t0Reset[mac5] != std::numeric_limits<uint32_t>::max())
        {
          ++boardsWithT0Reset;

          if(t0Reset[mac5] < t0ResetMin)
            t0ResetMin = t0Reset[mac5];

          if(t0Reset[mac5] > t0ResetMax)
            t0ResetMax = t0Reset[mac5];
        }

      if(nT1Resets[mac5] == 1 && t1Reset[mac5] != std::numeric_limits<uint32_t>::max())
        {
          ++boardsWithT1Reset;

	  // 78 is a bad board, nuke it
          if(t1Reset[mac5] < t1ResetMin && static_cast<int>(mac5) != 78 ) 
            t1ResetMin = t1Reset[mac5];

          if(t1Reset[mac5] > t1ResetMax && static_cast<int>(mac5) != 78 )
            t1ResetMax = t1Reset[mac5];
        }

      if(tdcT1Reset != std::numeric_limits<uint64_t>::max() && t1Reset[mac5] != std::numeric_limits<uint32_t>::max())
        {
          uint32_t fracTDCT1Reset = tdcT1Reset % static_cast<uint32_t>(1e9);
          uint64_t t1ResetTDCDiff = fracTDCT1Reset > t1Reset[mac5] ? fracTDCT1Reset - t1Reset[mac5] : t1Reset[mac5] - fracTDCT1Reset;

          if(fDebug) std::cout << "Sending metric T1ResetTDCDiff with value " << t1ResetTDCDiff << std::endl;
          sbndaq::sendMetric("CRT_board", mac5Str, "T1ResetTDCDiff", t1ResetTDCDiff, 0, artdaq::MetricMode::LastPoint);
        }

      for(int ch = 0; ch < 32; ++ch)
        {
          std::string chStr = std::to_string(mac5*100 + ch);
          if(fDebug) std::cout << "Sending metric ChReadoutRate with value " << chReadoutRate[mac5][ch] / fRateNormalisation << std::endl;
          sbndaq::sendMetric("CRT_channel", chStr, "ChReadoutRate", chReadoutRate[mac5][ch] / fRateNormalisation, 0, artdaq::MetricMode::Average);
        }
    } // loop over mac5

  /////////////////////////
  // Event-Level Metrics //
  /////////////////////////
  //Currently using "0" as my blank Mac5 address for the event-level metrics.

  if(boardsWithT0Reset > fBoardsRequiredForResetSpread)
    {
      uint64_t t0ResetSpread = t0ResetMax - t0ResetMin;
      if(fDebug) std::cout << "Sending metric T0ResetSpread with value " << t0ResetSpread << std::endl;
      sbndaq::sendMetric("CRT_event", "0", "T0ResetSpread", t0ResetSpread, 0, artdaq::MetricMode::Maximum);
    }

  if(boardsWithT1Reset > fBoardsRequiredForResetSpread)
    {
      uint64_t t1ResetSpread = t1ResetMax - t1ResetMin;
      if(fDebug) std::cout << "Sending metric T1ResetSpread with value " << t1ResetSpread << std::endl;
      sbndaq::sendMetric("CRT_event", "0", "T1ResetSpread", t1ResetSpread, 0, artdaq::MetricMode::Maximum);
    }
  
} //analyze

uint64_t sbndaq::BernCRTdqmSBND::GetRawEventTime(art::Event const &evt)
{
  art::Handle<artdaq::detail::RawEventHeader> DAQHeaderHandle;
  evt.getByLabel(fDAQHeaderModuleLabel, fDAQHeaderInstanceLabel, DAQHeaderHandle);

  if(DAQHeaderHandle.isValid())
    {
      artdaq::RawEvent rawHeaderEvent = artdaq::RawEvent(*DAQHeaderHandle);
      return rawHeaderEvent.timestamp() - fRawTSCorrection;
    }

  return std::numeric_limits<uint64_t>::max();
}

uint64_t sbndaq::BernCRTdqmSBND::GetSPECTDCT1ResetTime(art::Event const &evt, const uint64_t &rawEventTS)
{
  uint64_t minDiff   = std::numeric_limits<uint64_t>::max();
  uint64_t timestamp = 0;
  uint16_t count     = 0;

  for(const std::string &SPECTDCInstanceLabel : fSPECTDCInstanceLabels)
    {
      art::Handle<std::vector<artdaq::Fragment>> fragmentHandle;
      evt.getByLabel(fSPECTDCModuleLabel, SPECTDCInstanceLabel, fragmentHandle);

      if(!fragmentHandle.isValid() || fragmentHandle->size() == 0)
        continue;

      if(fragmentHandle->front().type() == artdaq::Fragment::ContainerFragmentType)
        {
          for(auto cont : *fragmentHandle)
            {
              artdaq::ContainerFragment contf(cont);

              if(contf.fragment_type() == sbndaq::detail::FragmentType::TDCTIMESTAMP)
                {
                  for(unsigned i = 0; i < contf.block_count(); ++i)
                    {
                      const artdaq::Fragment frag                = *contf[i].get();
                      const sbndaq::TDCTimestampFragment tdcFrag = sbndaq::TDCTimestampFragment(frag);
                      const sbndaq::TDCTimestamp         *tdcTS  = tdcFrag.getTDCTimestamp();

                      if(tdcTS->vals.channel == fSPECTDCT1Channel)
                        {
                          uint64_t diff = tdcTS->timestamp_ns() > rawEventTS ? tdcTS->timestamp_ns() - rawEventTS : rawEventTS - tdcTS->timestamp_ns();

                          if(diff < minDiff)
                            {
                              minDiff   = diff;
                              timestamp = tdcTS->timestamp_ns();
                            }

                          ++count;
                        }
                    }
                }
            }
        }
      else if(fragmentHandle->front().type() == sbndaq::detail::FragmentType::TDCTIMESTAMP)
        {
          for(auto frag : *fragmentHandle)
            {
              const sbndaq::TDCTimestampFragment tdcFrag = sbndaq::TDCTimestampFragment(frag);
              const sbndaq::TDCTimestamp         *tdcTS  = tdcFrag.getTDCTimestamp();

              if(tdcTS->vals.channel == fSPECTDCT1Channel)
                {
                  uint64_t diff = tdcTS->timestamp_ns() > rawEventTS ? tdcTS->timestamp_ns() - rawEventTS : rawEventTS - tdcTS->timestamp_ns();

                  if(diff < minDiff)
                    {
                      minDiff   = diff;
                      timestamp = tdcTS->timestamp_ns();
                    }

                  ++count;
                }
            }
        }
    }

  if(count > 0)
    return timestamp;
  else
    return std::numeric_limits<uint64_t>::max();
}

void sbndaq::BernCRTdqmSBND::reconfigure(fhicl::ParameterSet const & pset)
{
  fDebug                        = pset.get<bool>("Debug", false);
  fCRTModuleLabel               = pset.get<std::string>("CRTModuleLabel");
  fCRTInstanceLabel             = pset.get<std::string>("CRTInstanceLabel");
  fSPECTDCModuleLabel           = pset.get<std::string>("SPECTDCModuleLabel");
  fSPECTDCInstanceLabels        = pset.get<std::vector<std::string>>("SPECTDCInstanceLabels");
  fDAQHeaderModuleLabel         = pset.get<std::string>("DAQHeaderModuleLabel");
  fDAQHeaderInstanceLabel       = pset.get<std::string>("DAQHeaderInstanceLabel");
  fSPECTDCT1Channel             = pset.get<uint16_t>("SPECTDCT1Channel");
  fRawTSCorrection              = pset.get<uint64_t>("RawTSCorrection");
  fBigHitADCThreshold           = pset.get<uint16_t>("BigHitADCThreshold");
  fBoardsRequiredForResetSpread = pset.get<uint16_t>("BoardsRequiredForResetSpread");
  fRateNormalisation            = pset.get<double>("RateNormalisation");
  fMac5s                        = pset.get<std::vector<uint8_t>>("metric_board_config.groups.CRT_board");
  fBeamWindowStart = pset.get<int>("BeamWindowStart",320000);
  fBeamWindowEnd = pset.get<int>("BeamWindowEnd",350000);
 
} //reconfigure

DEFINE_ART_MODULE(sbndaq::BernCRTdqmSBND)
//this is where the name is specified
