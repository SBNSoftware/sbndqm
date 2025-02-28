////////////////////////////////////////////////////////////////////////
// Class:       BernCRTdqmSBND
// Module Type: analyzer
// File:        BernCRTdqmSBND_module.cc
// Description:
// Modified by Matt King January 2024 for use on SBND
// mking9@uchicago.edu
// Additionally modified by Nikki Pallat. Last update made on November 18 2024.
// palla110@umn.edu
//
// This Module sends metrics from the SBND CRT modules to the redis database
// 
// Current metrics being monitored:
//	Channel-level:
//		ADC - the ADC value for a hit on a channel
//		lastbighit - the time on a given hit since the last hit above 600 ADC threshold
//		pedestalMean - pedestal mean for a channel; pedestal calculated summing all ADC for a given channel < 4000 
//		pedestalRMS2 - pedestal RMS squared
//		pedestalRMS - pedestal RMS
//		ChFlag3Rate - Number of flag 3 hit rate for each channel
//	Board-level:
//		MaxADCValue - Maximum ADC value across all channels on board
//		MaxADCChannel - Channel which has the maximum ADC Value - given as index 0-31 + 32*mac5 (absolute channel reference)
//		Baseline - average of channels across board, not including the maximum 2 channel values (cut out possible signals)
//		T0 - T0 timestamp of a hit
//		T1 - T1 timestamp of a hit
//		T0Clockdrift - For T0 reset event, difference of T0 timestamp from pps
//		T1Clockdrift - For a T1 reset event, difference of T1 timestamp from beam signal (NEED TO IMPLEMENT)
//		Earlysynch - Difference of timestamp to beginning of pull window
//		Latesynch - Difference of timestamp to end of pull window
//		Flag3Hit - Flag 3 hit rate for all channels in the board
//		Deadtime - time following any type of hit (of any flag) where the board cannot process another hit (time difference between consecutive hits  on the same board)
//              MissingT0 - counter of missing T0 reset (flag is not 1, 3, 7, or 11)
//              MissingT1 - counter of missing T1 reset (flag is not 3, 7, 10, or 11)
//	Fragment-Level:
//		Flag - flag of the fragment
//		frag_count - number of fragments sent 
//		zero_rate - number of empty fragments sent
//	Event-Level (mostly for offline monitoring of artroot events):
//		num_fragments - number of fragments sent in the event
//		num_hits - number of hits across all fragments in the event
//      Event-level:
//               T0ResetSpread  - The range between the lowest & highest T0 values for T0 reset events seen across all boards
//               T1ResetSpread  - The range between the lowest & highest T0 values for T1 reset events seen across all boards
//
// To-do:
//	1. Make sure we handle different CRT walls with overlapping mac5 addresses properly
//	2. Turn lastbighit threshold into a fcl parameter
//	3. Include beam timing information to make T1Clockdrift useful		
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
//add these
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
//---
//#include "art/Framework/Services/Optional/TFileService.h"

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

// See StackOverflow 5966594
#define StringSize( L )    #L
#define MakeString( M, L ) M(L)
#define $Line MakeString( StringSize, __LINE__ )
#define Hello __FILE__ "(" $Line ") : Pou sai file - "

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

#pragma message(Hello "got the private variables")

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

  bool IsSideCRT(const icarus::crt::BernCRTTranslator & hit);

   uint64_t lastbighit[32];
   float pedSum[32];
   float pedMax[32];
   float ped2Max[32];
   float pedSumSq[32];
   float pedNHits[32];
   float flag3channel[32];
   //float NHits[32];

  bool debug = false;

  //sample histogram
  TH1F* fSampleHist;
  
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
  //sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics")); //This causes the error for no "metrics" at the beginning or the end
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_fragment_config"));

  this->reconfigure( pset );
}

sbndaq::BernCRTdqmSBND::~BernCRTdqmSBND()
{
}

bool sbndaq::BernCRTdqmSBND::IsSideCRT(const icarus::crt::BernCRTTranslator & hit) {
  /**
   * Fragment ID described in SBN doc 16111
   */
  return (hit.fragment_ID & 0x3100) == 0x3100;
}


void sbndaq::BernCRTdqmSBND::analyze(art::Event const & evt) {
  //sleep(2);

  if (debug) std::cout << "######################################################################" << std::endl;
  if (debug) std::cout << std::endl;  
  if (debug) std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();

#pragma message(Hello "custom analyze here")

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
  /**
  * We are using BernCRTTranslatorV2, which takes in a fragment and stores its corresponding fields in a new object
  * Each BernCRTTranslator object contains the information from 1 CRT hit, and this hit_vector contains the information
  * From all of the hits in all of the fragments within an art event. -MK
  */
  
  if (debug) std::cout<<"Hit vector declared. Going to getMany fragments";

  //auto fragmentHandles = evt.getMany<artdaq::Fragments>();
  
std::string fCRTModuleLabel = "daq";
std::string CRTInstanceLabel = "ContainerBERNCRTV2";

art::Handle<std::vector<artdaq::Fragment>> fragmentHandle;
evt.getByLabel(fCRTModuleLabel, CRTInstanceLabel, fragmentHandle);


if (debug) std::cout<<"evt.getByLabel successful.";

if(!fragmentHandle.isValid() || fragmentHandle->size() == 0)
        return;

 // if (debug) std::cout<<"fragmentHandles gotten. Looping over fragmentHandles";

  //for (auto  handle : fragmentHandles) {
  //  if (!handle.isValid() || handle->size() == 0){
  //    if (debug) {std::cout << "Fragment handle is not valid or handle size is 0";}
  //    continue;}

    auto this_hit_vector = icarus::crt::BernCRTTranslator::getCRTData(*fragmentHandle);
    if (debug) std::cout<<"Successfully obtained CRT data" << std::endl;
    
    /////////////////////////////////
    // Send Fragment Level Metrics //
    /////////////////////////////////
    
    //Copied from FragmentDQMAna_module.cc
    
    for (auto const& frag : *fragmentHandle){
      //frag is artdaq::Fragment

      // if fragment is a container fragment, print # of fragments in that container fragment
      if(frag.type() != artdaq::Fragment::ContainerFragmentType) {
        if (debug) std::cout<<"Fragment type is incorrect!";
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
      else if (cont_frag.fragment_type() == sbndaq::detail::FragmentType::BERNCRTV2) {group_name = "CRT_cont_frag";} //this one is relevant for us
	//print out arguments of the sendMetric line
	//i.e. print out fragment_id to match to fcl
	if (debug) std::cout<<"fragment_id: "<<fragment_id<<std::endl;

      sbndaq::sendMetric(group_name, fragment_id, "frag_count", frag_count, 0, artdaq::MetricMode::Average);
      sbndaq::sendMetric(group_name, fragment_id, "zero_rate", nzero, 0, artdaq::MetricMode::Rate);
     
     }//end loop over handle
    
      //End copy from FragmentDQMAna_module.cc

    //Concatenate hit vectors from each fragment into an event-long hit vector.
    hit_vector.insert(hit_vector.end(),this_hit_vector.begin(),this_hit_vector.end());
  //}//loop over fragment handles ???
  
    ///////////////////////////////////////
    // Extract Information from the Hits //
    ///////////////////////////////////////
  
  //Event-level variables for art root events - basic checks unnecessary for online monitoring
  size_t num_fragments = fragmentHandle->size();
  size_t num_hits = hit_vector.size();
  
  //Variables used in Grafana to be sent to DQM OM:
  size_t num_t1_resets = 0;
  size_t hitsperplane[7] = {0,0,0,0,0,0,0};

  //Initialize variables used to calculate pedestals and flag3
  for(int c=0; c<32; c++){
    sbndaq::BernCRTdqmSBND::pedSum[c] = 0.; 
    pedMax[c] = 0.; 
    ped2Max[c] = 0.; 
    sbndaq::BernCRTdqmSBND::pedSumSq[c] = 0.; 
    sbndaq::BernCRTdqmSBND::pedNHits[c] = 0.; 
    sbndaq::BernCRTdqmSBND::flag3channel[c] = 0.; 
    //sbndaq::BernCRTdqmSBND::NHits[c] = 0.; 
  }

  // Initialize variables used to calculate deadtime
  int count_hit = 0;
  uint64_t deadtime;
  //uint64_t & prev_fragment_timestamp = hit.timestamp;
  uint64_t prev_fragment_timestamp;

  // Flag 3 hits
  uint64_t flag3hit = 0;
  //uint64_t missingT0 = 0;
  //uint64_t missingT1 = 0;


  //loop over all CRT hits in an event
  for(const auto & hit : hit_vector) {

    // Extract core hit information
      const uint8_t& mac5 = hit.mac5;
      //const uint32_t ts0  = hit.ts0;
      //const uint16_t* adc = hit.adc;

      // Extract metadata information
#pragma message(Hello "RETHERE - restore fragmentTS")
      //const uint64_t& fragmentTS = hit.timestamp;
      const bool isTs0Reset      = hit.IsReference_TS0();
      const bool isTs1Reset      = hit.IsReference_TS1();
      const bool ts0Good         = !hit.IsOverflow_TS0();
      const bool ts1Good         = !hit.IsOverflow_TS1();

      std::string mac5Str = std::to_string(mac5);
      //if(fDebug) std::cout << "Mac5: " << mac5Str <<std::endl;

    enum Detector {SIDE_CRT, TOP_CRT};
//    const Detector detector = IsSideCRT(hit) ? SIDE_CRT : TOP_CRT;
    const uint16_t & fragment_id        = hit.fragment_ID;
    /**
     * TODO:
     * In order to distinguish between Top and Side CRT
     * use the variable detector, defined above
     * Otherwise, MAC address alone is not sufficient,
     * as some MACs overlap between Top and Side
     *
     * Alternative: use fragment_ID directly (fragment_IDs
     * are unique)
     */

    const uint64_t & fragment_timestamp = hit.timestamp;

    //data from FEB:
    //const uint8_t & mac5     = hit.mac5;
    unsigned readout_number  = hit.mac5;
    std::string readout_number_str = std::to_string(readout_number);

    //std::cout<<"Mac5: "<<readout_number_str<<std::endl;

    //store the timing and flag information from a hit
    const uint32_t ts0      = static_cast<uint32_t>(hit.ts0);
    const uint32_t ts1      = static_cast<uint32_t>(hit.ts1);
    const bool     isTS0    = hit.IsReference_TS0();
    const bool     isTS1    = hit.IsReference_TS1();
    const bool     isTS0good=!hit.IsOverflow_TS0();
    const bool     isTS1good=!hit.IsOverflow_TS1();
    
    // Deadtime
    if (count_hit == 0) prev_fragment_timestamp = fragment_timestamp;
    if (count_hit != 0) {
        deadtime = fragment_timestamp - prev_fragment_timestamp;
    }
    prev_fragment_timestamp = hit.timestamp;
    count_hit++;

    const uint16_t * adc = hit.adc;

    for(int ch=0; ch<32; ch++) {
      if( adc[ch] > 600 ) {
        sbndaq::BernCRTdqmSBND::lastbighit[ch] = fragment_timestamp;
      }
    }
    const uint64_t & this_poll_end             = hit.this_poll_end;
    const uint64_t & last_poll_start           = hit.last_poll_start;

    size_t max        = 0;
    size_t secondmax  = 0;
    size_t totaladc   = 0;
    size_t ADCchannel = 0;

    std::string FEBID_str = std::to_string(fragment_id);
    sbndaq::sendMetric("CRT_board", FEBID_str, "FEBID", fragment_id, 0, artdaq::MetricMode::LastPoint); 


    //let's fill our sample hist with the Time_TS0()-1e9 if 
    //it's a GPS reference pulse
    if(isTS0){
      if (debug) std::cout<<" TS0 "<<ts0 - 1e9<<std::endl;
    }
    if(isTS1){
      if (debug) std::cout<<" TS1 "<<ts1 - 1e9<<std::endl; 
      num_t1_resets++;
    }
    
    ///////////////////////////
    // Channel-Level Metrics //
    ///////////////////////////
   
    auto currflag = hit.flags;
    int maxindex = -1;
    for(int i = 0; i<32; i++) {
      if (currflag == 3) {
        sbndaq::BernCRTdqmSBND::flag3channel[i]++;
        flag3hit++;
      }
      totaladc  += adc[i];
      ADCchannel = adc[i];
      sbndaq::BernCRTdqmSBND::pedSum[i] += adc[i];
      if (adc[i] > pedMax[i]) {pedMax[i] = adc[i];}
      if (adc[i] > ped2Max[i]) {
        if (adc[i] < pedMax[i]) {
          ped2Max[i] += adc[i];
        }
      }
      sbndaq::BernCRTdqmSBND::pedSumSq[i] += adc[i]*adc[i];
      sbndaq::BernCRTdqmSBND::pedNHits[i]++;
      uint64_t lastbighitchannel = fragment_timestamp -sbndaq::BernCRTdqmSBND::lastbighit[i];
      /////    RMSchannel = rms[i];
      
      //Send Channel-Level Metrics to the database
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average); 
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "lastbighit", lastbighitchannel, 0, artdaq::MetricMode::Average);
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ChFlag3Rate", flag3channel[i], 0, artdaq::MetricMode::Average);
      // Pedestals
      double pedestalMean = sbndaq::BernCRTdqmSBND::pedSum[i] - sbndaq::BernCRTdqmSBND::pedMax[i] - sbndaq::BernCRTdqmSBND::ped2Max[i];
      sbndaq::BernCRTdqmSBND::pedSumSq[i]= sbndaq::BernCRTdqmSBND::pedSumSq[i] - sbndaq::BernCRTdqmSBND::pedMax[i]*sbndaq::BernCRTdqmSBND::pedMax[i] - sbndaq::BernCRTdqmSBND::ped2Max[i]*sbndaq::BernCRTdqmSBND::ped2Max[i];
      double pedMeanRMS = pedestalMean/sbndaq::BernCRTdqmSBND::pedNHits[i];
      // need to modify
      //double pedestalRMS2 = sbndaq::BernCRTdqmSBND::pedNHits[i] * pedMeanRMS*pedMeanRMS - 2 * pedMeanRMS*sbndaq::BernCRTdqmSBND::pedSum[i] + sbndaq::BernCRTdqmSBND::pedSumSq[i];
      double pedestalRMS2 = sbndaq::BernCRTdqmSBND::pedNHits[i] * pedMeanRMS*pedMeanRMS - 2 * pedestalMean + sbndaq::BernCRTdqmSBND::pedSumSq[i];
      double pedestalRMS = sqrt(pedestalRMS2/sbndaq::BernCRTdqmSBND::pedNHits[i]);
      // Send Metrics to the database **
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "pedestalMean", pedestalMean, 0, artdaq::MetricMode::Average); 
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "pedestalRMS2", pedestalRMS2, 0, artdaq::MetricMode::Average); 
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "pedestalRMS", pedestalRMS, 0, artdaq::MetricMode::Average); 
      
    /////////////////////////
    // Board-Level Metrics //
    /////////////////////////

      if(adc[i] > max){
        max = adc[i];
	maxindex = i;
      }
      
    }
    
    //calculate second max:
    for(int i = 0; i<32; i++) {
      if(i == maxindex) {continue;}
      if(adc[i] > secondmax) {
      	secondmax = adc[i];
      } 
    
    }
    
    //old definition of baseline:
    //int baseline = (totaladc-max)/31;
    
    // need to redefine baseline to take out the second maximum (one max for each board) -MK
    int baseline = (totaladc - max - secondmax)/30;

    uint64_t earlysynch = last_poll_start - fragment_timestamp;
    uint64_t latesynch = fragment_timestamp - this_poll_end;
    
    //From the code that writes to Grafana	
    auto thisone = hit.fragment_ID;  uint plane = (thisone & 0x0700) >> 8;
    
    if (debug) std::cout<<"Plane: "<<plane<<std::endl;
    
    if (plane>7) {if (debug) std::cout << "bad plane value " << plane << std::endl; plane=0;}
  
    auto thisflag = hit.flags;
    /*
    if (thisflag != 7 && thisflag != 11 && thisflag != 3 && thisflag != 1) {
      missingT1++;
    }
    if (thisflag != 7 && thisflag != 11 && thisflag != 3 && thisflag != 10) {
      missingT0++;
    }
    */
    
#pragma message(Hello "I have changed the way missing T0/1 is done")
    if(!ts0Good)
      ++missingT0[mac5];
    
    if(!ts1Good)
      ++missingT1[mac5];

    // require that this is data and not clock reset (0xC), and that the ts1 time is valid (0x2)
    if (thisflag & 0x2 && !(thisflag & 0xC) ) {
      // check ts1 for beam window
      if(debug) std::cout<<"It's a data event! Ts1: "<<ts1<<std::endl;
      if ((int)ts1>fBeamWindowStart && (int)ts1<fBeamWindowEnd) hitsperplane[plane]++;
    }

    if(isTs0Reset)
        {
          ++nT0Resets[mac5];

          if(ts0Good)
            t0Reset[mac5] = ts0;
        }

    if(isTs1Reset)
      {
	++nT1Resets[mac5];
	
	if(ts0Good)
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
    
    /**
    * Below we send the metric information, hit by hit, to the online monitor / DQM.
    * The syntax for the sendMetric function is as follows (from SBNMetricManager.hh):
    *
    * void sendMetric(std::string const& group, 
                      std::string const& instance, 
    		      std::string const& metric, 
                      long unsigned int const& value, 
		      int level, 
		      MetricMode mode, 
		      std::string const& metricPrefix = "", 
		      bool useNameOverride = false) {
  		if (metricMan != NULL) {
   		 metricMan->sendMetric(buildMetricName(group, instance, metric), value, "", level, mode, metricPrefix, useNameOverride);
  		}
	}
    *
    * Further documentation on this functon from MetricManager.hh:
    * * \brief Send a metric with the given parameters to any MetricPlugins with a threshold level >= to level.
	 * \param name The Name of the metric
	 * \param value The value of the metric
	 * \param unit The units of the metric
	 * \param level The verbosity level of the metric. Higher number == more verbose
	 * \param mode The MetricMode that the metric should operate in. Options are:
	 *    LastPoint: Every reporting_interval, the latest metric value is sent (For run/event numbers, etc)
	 *    Accumulate: Every reporting_interval, the sum of all metric values since the last report is sent (for counters)
	 *    Average: Every reporting_interval, the average of all metric values since the last report is sent (for rates)
	
    * We note that the level here is used as a threshold; we send any value of the metric above the level. For the CRT DQM, all levels = 0.
    */

    sbndaq::sendMetric("CRT_board", readout_number_str, "MaxADCValue", max, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "MaxADCChannel", maxindex + 32 * mac5, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "Flag", thisflag, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "baseline", baseline, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS0", static_cast<int>(ts0), 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS1", static_cast<int>(ts1), 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "Deadtime", deadtime, 0, artdaq::MetricMode::Minimum);
    sbndaq::sendMetric("CRT_board", readout_number_str, "MissingT0", missingT0[mac5], 0, artdaq::MetricMode::Maximum);
    sbndaq::sendMetric("CRT_board", readout_number_str, "MissingT1", missingT1[mac5], 0, artdaq::MetricMode::Maximum);
 
    //only send clockdrift info when it makes sense to do so; that is, for T0 reset events.
#pragma message(Hello "I have NOT changed the clock drift quantities, RETHERE!")
    if(isTS0 && isTS0good) {sbndaq::sendMetric("CRT_board", readout_number_str, "T0clockdrift", ts0 - 1e9, 0, artdaq::MetricMode::LastPoint);}
    if(isTS1 && isTS1good) {sbndaq::sendMetric("CRT_board", readout_number_str, "T1clockdrift", ts1 - 1e9, 0, artdaq::MetricMode::LastPoint);}

    //Sychronization Metrics
    sbndaq::sendMetric("CRT_board", readout_number_str, "earlysynch", earlysynch, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric("CRT_board", readout_number_str, "latesynch", latesynch, 0, artdaq::MetricMode::Average);

    // Flag 3 Hits (Board Level)
    sbndaq::sendMetric("CRT_board", readout_number_str, "Flag3Hit", flag3hit, 0, artdaq::MetricMode::Average);  

  } //loop over all CRT hits in an event

  /*
   * RETHERE: My spicy metrics live here
   */
#pragma message(Hello "EDW SE 8ELW MASTORA")

  uint32_t t0ResetMin = std::numeric_limits<uint32_t>::max();
  uint32_t t0ResetMax = std::numeric_limits<uint32_t>::lowest();
  uint32_t t1ResetMin = std::numeric_limits<uint32_t>::max();
  uint32_t t1ResetMax = std::numeric_limits<uint32_t>::lowest();

  uint16_t boardsWithT0Reset = 0;
  uint16_t boardsWithT1Reset = 0;
 
  for(const uint8_t& mac5 : fMac5s)
    {
      std::string mac5Str = std::to_string(mac5);

      if(fDebug) std::cout << "Sending metric MissingT0 with value " << missingT0[mac5] << std::endl;
      //sbndaq::sendMetric("CRT_board", mac5Str, "MissingT0", missingT0[mac5], 0, artdaq::MetricMode::Accumulate);

      if(fDebug) std::cout << "Sending metric MissingT1 with value " << missingT1[mac5] << std::endl;
      //sbndaq::sendMetric("CRT_board", mac5Str, "MissingT1", missingT1[mac5], 0, artdaq::MetricMode::Accumulate);

      if(fDebug) std::cout << "Sending metric ReadoutRate with value " << readoutRate[mac5]  / fRateNormalisation << std::endl;
      //sbndaq::sendMetric("CRT_board", mac5Str, "ReadoutRate", readoutRate[mac5] / fRateNormalisation, 0, artdaq::MetricMode::Average);

      if(fDebug) std::cout << "Sending metric PullWindow with value " << maxTS[mac5] - minTS[mac5] << std::endl;
      //sbndaq::sendMetric("CRT_board", mac5Str, "PullWindow", maxTS[mac5] - minTS[mac5], 0, artdaq::MetricMode::Maximum);

      if(fDebug) std::cout << "Sending metric NT0Resets with value " << nT0Resets[mac5] << std::endl;
      //sbndaq::sendMetric("CRT_board", mac5Str, "NT0Resets", nT0Resets[mac5], 0, artdaq::MetricMode::Maximum);

      if(fDebug) std::cout << "Sending metric NT1Resets with value " << nT1Resets[mac5] << std::endl;
      //sbndaq::sendMetric("CRT_board", mac5Str, "NT1Resets", nT1Resets[mac5], 0, artdaq::MetricMode::Maximum);

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

          if(t1Reset[mac5] < t1ResetMin)
            t1ResetMin = t1Reset[mac5];

          if(t1Reset[mac5] > t1ResetMax)
            t1ResetMax = t1Reset[mac5];
        }

      if(tdcT1Reset != std::numeric_limits<uint64_t>::max() && t1Reset[mac5] != std::numeric_limits<uint32_t>::max())
        {
          uint32_t fracTDCT1Reset = tdcT1Reset % static_cast<uint32_t>(1e9);
          uint64_t t1ResetTDCDiff = fracTDCT1Reset > t1Reset[mac5] ? fracTDCT1Reset - t1Reset[mac5] : t1Reset[mac5] - fracTDCT1Reset;

          if(fDebug) std::cout << "Sending metric T1ResetTDCDiff with value " << t1ResetTDCDiff << std::endl;
          //sbndaq::sendMetric("CRT_board", mac5Str, "T1ResetTDCDiff", t1ResetTDCDiff, 0, artdaq::MetricMode::LastPoint);
        }

      for(int ch = 0; ch < 32; ++ch)
        {
          std::string chStr = std::to_string(mac5*100 + ch);
          //if(fDebug) std::cout << "Sending metric ChReadoutRate with value " << chReadoutRate[mac5][ch] / fRateNormalisation << std::endl;
          //sbndaq::sendMetric("CRT_channel", chStr, "ChReadoutRate", chReadoutRate[mac5][ch] / fRateNormalisation, 0, artdaq::MetricMode::Average);
        }
    } // loop over mac5

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
  
 
    /////////////////////////
    // Event-Level Metrics //
    /////////////////////////
  //Currently using "0" as my blank Mac5 address for the event-level metrics.
  
  //Metrics which are on the Grafana:
  
 	 //"CRT hits in beam window per plane per event"
    for (int i=0;i<7;++i){
    	if(debug) {std::cout<<"hitsperplane["<<i<<"]: "<<hitsperplane[i]<<std::endl;}
        sbndaq::sendMetric("CRT_event", std::to_string(0),
            std::string("CRT_hits_beam_plane_")+std::to_string(i),
            hitsperplane[i],
            0, artdaq::MetricMode::LastPoint);
      }
	  //"CRT T1 resets per event"
	  if(debug) {std::cout<<"num_t1_resets: "<<num_t1_resets<<std::endl;}
    sbndaq::sendMetric("CRT_event", std::to_string(0),
          "T1_resets_per_event",
          num_t1_resets,
          0, artdaq::MetricMode::LastPoint);
	  
  //Other event-level metrics:
  sbndaq::sendMetric("CRT_event", std::to_string(0), "num_fragments", num_fragments, 0, artdaq::MetricMode::LastPoint);
  sbndaq::sendMetric("CRT_event", std::to_string(0), "num_hits", num_hits, 0, artdaq::MetricMode::LastPoint);


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
