////////////////////////////////////////////////////////////////////////
// Class:       BernCRTdqm
// Module Type: analyzer
// File:        BernCRTdqm_module.cc
// Description:
// Modified by Matt King January 2024 for use on SBND
// mking9@uchicago.edu
//
// This Module sends metrics from the SBND CRT modules to the redis database
// 
// Current metrics being monitored:
//	Channel-level:
//		ADC - the ADC value for a hit on a channel
//		lastbighit - the time on a given hit since the last hit above 600 ADC threshold
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
//	Fragment-Level:
//		Flag - flag of the fragment
//		frag_count - number of fragments sent 
//		zero_rate - number of empty fragments sent
//	Event-Level (mostly for offline monitoring of artroot events):
//		num_fragments - number of fragments sent in the event
//		num_hits - number of hits across all fragments in the event
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
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
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

namespace sbndaq {
  class BernCRTdqm;
}

/*****/

class sbndaq::BernCRTdqm : public art::EDAnalyzer {

public:
  explicit BernCRTdqm(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~BernCRTdqm();
  
  virtual void analyze(art::Event const & evt);
  void reconfigure(fhicl::ParameterSet const & pset);
 
private:
  bool IsSideCRT(const icarus::crt::BernCRTTranslator & hit);

   uint64_t lastbighit[32];
   float pedSum[32];
   float pedSumSq[32];
   float pedNHits[32];

  bool debug = false;

  //sample histogram
  TH1F* fSampleHist;
  
  //fhicl parameters
  int fBeamWindowStart;
  int fBeamWindowEnd;
  
};

//Define the constructor
sbndaq::BernCRTdqm::BernCRTdqm(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  //sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics")); //This causes the error for no "metrics" at the beginning or the end
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));
  
  this->reconfigure( pset );
}

sbndaq::BernCRTdqm::~BernCRTdqm()
{
}

bool sbndaq::BernCRTdqm::IsSideCRT(const icarus::crt::BernCRTTranslator & hit) {
  /**
   * Fragment ID described in SBN doc 16111
   */
  return (hit.fragment_ID & 0x3100) == 0x3100;
}


void sbndaq::BernCRTdqm::analyze(art::Event const & evt) {
  //sleep(2);

  if (debug) std::cout << "######################################################################" << std::endl;
  if (debug) std::cout << std::endl;  
  if (debug) std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();

  std::vector<icarus::crt::BernCRTTranslator> hit_vector;
  /**
  * We are using BernCRTTranslatorV2, which takes in a fragment and stores its corresponding fields in a new object
  * Each BernCRTTranslator object contains the information from 1 CRT hit, and this hit_vector contains the information
  * From all of the hits in all of the fragments within an art event. -MK
  */
  
  auto fragmentHandles = evt.getMany<artdaq::Fragments>();
  for (auto  handle : fragmentHandles) {
    if (!handle.isValid() || handle->size() == 0)
      continue;

    auto this_hit_vector = icarus::crt::BernCRTTranslator::getCRTData(*handle);
    
    /////////////////////////////////
    // Send Fragment Level Metrics //
    /////////////////////////////////
    
    //Copied from FragmentDQMAna_module.cc
    
    // Variables used to calculate pedestals
    //map<int, vector<double> > sum, sumsq;
    //map<int, vector<int> > nhits;

    for (auto const& frag : *handle){
      //frag is artdaq::Fragment

      // if fragment is a container fragment, print # of fragments in that container fragment
      if(frag.type() != artdaq::Fragment::ContainerFragmentType)
        continue;

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

        /*/Initialize variables used to calculate pedestals
        for(int c=0; c<32; c++){
          sum[fragment_id].push_back(0.); 
          sumsq[fragment_id].push_back(0.); 
          nhits[fragment_id].push_back(0); 
        }*/

      sbndaq::sendMetric(group_name, fragment_id, "frag_count", frag_count, 0, artdaq::MetricMode::Average);
      sbndaq::sendMetric(group_name, fragment_id, "zero_rate", nzero, 0, artdaq::MetricMode::Rate);
     
     }//end loop over handle
    
      //End copy from FragmentDQMAna_module.cc

    //Concatenate hit vectors from each fragment into an event-long hit vector.
    hit_vector.insert(hit_vector.end(),this_hit_vector.begin(),this_hit_vector.end());
  }
  
    ///////////////////////////////////////
    // Extract Information from the Hits //
    ///////////////////////////////////////
  
  //Event-level variables for art root events - basic checks unnecessary for online monitoring
  size_t num_fragments = fragmentHandles.size();
  size_t num_hits = hit_vector.size();
  
  //Variables used in Grafana to be sent to DQM OM:
  size_t num_t1_resets = 0;
  size_t hitsperplane[7] = {0,0,0,0,0,0,0};

  //Initialize variables used to calculate pedestals
  for(int c=0; c<32; c++){
    sbndaq::BernCRTdqm::pedSum[c] = 0.; 
    sbndaq::BernCRTdqm::pedSumSq[c] = 0.; 
    sbndaq::BernCRTdqm::pedNHits[c] = 0.; 
  }

  //loop over all CRT hits in an event
  for(const auto & hit : hit_vector) {

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
    const uint8_t & mac5     = hit.mac5;
    unsigned readout_number  = mac5;
    std::string readout_number_str = std::to_string(readout_number);
    
    //std::cout<<"Mac5: "<<readout_number_str<<std::endl;

    //store the timing and flag information from a hit
    const int ts0      = hit.ts0;
    const int ts1      = hit.ts1;
    const bool     isTS0    = hit.IsReference_TS0();
    const bool     isTS1    = hit.IsReference_TS1();
    const bool     isTS0good=!hit.IsOverflow_TS0();
    const bool     isTS1good=!hit.IsOverflow_TS1();

    const uint16_t * adc = hit.adc;

    for(int ch=0; ch<32; ch++) {
      if( adc[ch] > 600 ) {
        sbndaq::BernCRTdqm::lastbighit[ch] = fragment_timestamp;
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
    
    int maxindex = -1;
    for(int i = 0; i<32; i++) {
      totaladc  += adc[i];
      ADCchannel = adc[i];
      if (adc[i] < 4000) { 
        sbndaq::BernCRTdqm::pedSum[i] += adc[i];
        sbndaq::BernCRTdqm::pedSumSq[i] += adc[i]*adc[i];
        sbndaq::BernCRTdqm::pedNHits[i]++;
        //sum[fragment_id][i] += adc[i];
        //sumsq[fragment_id][i] += adc[i];
        //nhits[fragment_id][i]++;
      }
      uint64_t lastbighitchannel = fragment_timestamp -sbndaq::BernCRTdqm::lastbighit[i];
      /////    RMSchannel = rms[i];
      
      //Send Channel-Level Metrics to the database
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "ADC", ADCchannel, 0, artdaq::MetricMode::Average); 
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "lastbighit", lastbighitchannel, 0, artdaq::MetricMode::Average);
      // Pedestals
      //if nhits[fragment_id][i] > 4000 {}
      /*double pedestalMean = sum[fragment_id][i]/nhits[fragment_id][i];
      double pedestalRMS2 = nhits[fragment_id][i]* pedestalMean*pedestalMean - 2 * pedestalMean*sum[fragment_id][i] + sumsq[fragment_id][i];
      double pedestalRMS = sqrt(pedestalRMS2/nhits[fragment_id][i]);*/
      double pedestalMean = sbndaq::BernCRTdqm::pedSum[i]/sbndaq::BernCRTdqm::pedNHits[i];
      double pedestalRMS2 = sbndaq::BernCRTdqm::pedNHits[i] * pedestalMean*pedestalMean - 2 * pedestalMean*sbndaq::BernCRTdqm::pedSum[i] + sbndaq::BernCRTdqm::pedSumSq[i];
      double pedestalRMS = sqrt(pedestalRMS2/sbndaq::BernCRTdqm::pedNHits[i]);
      // Send Metrics to the database **
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "pedestalMean", pedestalMean, 0, artdaq::MetricMode::Average); 
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "pedestalRMS2", pedestalRMS2, 0, artdaq::MetricMode::Average); 
      sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "pedestalRMS", pedestalRMS, 0, artdaq::MetricMode::Average); 
      //sbndaq::sendMetric("CRT_channel", std::to_string(i), "ADC", ADCchannel, 0, artdaq::MetricMode::Average); 
      //sbndaq::sendMetric("CRT_channel", std::to_string(i), "lastbighit", lastbighitchannel, 0, artdaq::MetricMode::Average); 
      
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
    // require that this is data and not clock reset (0xC), and that the ts1 time is valid (0x2)
    if (thisflag & 0x2 && !(thisflag & 0xC) ) {
      // check ts1 for beam window
      if(debug) std::cout<<"It's a data event! Ts1: "<<ts1<<std::endl;
      if ((int)ts1>fBeamWindowStart && (int)ts1<fBeamWindowEnd) hitsperplane[plane]++;
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
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS0", ts0, 0, artdaq::MetricMode::LastPoint);
    sbndaq::sendMetric("CRT_board", readout_number_str, "TS1", ts1, 0, artdaq::MetricMode::LastPoint);
    
    //only send clockdrift info when it makes sense to do so; that is, for T0 reset events.
    if(isTS0 && isTS0good) {sbndaq::sendMetric("CRT_board", readout_number_str, "T0clockdrift", ts0 - 1e9, 0, artdaq::MetricMode::LastPoint);}
    if(isTS1 && isTS1good) {sbndaq::sendMetric("CRT_board", readout_number_str, "T1clockdrift", ts1 - 1e9, 0, artdaq::MetricMode::LastPoint);}

    //Sychronization Metrics
    sbndaq::sendMetric("CRT_board", readout_number_str, "earlysynch", earlysynch, 0, artdaq::MetricMode::Average);
    sbndaq::sendMetric("CRT_board", readout_number_str, "latesynch", latesynch, 0, artdaq::MetricMode::Average);
    
  } //loop over all CRT hits in an event
 
  /*/ Pedestals (only calculate each event)
  double mean = sum[fragment_id][i]/nhits[fragment_id][i];
  double rms2 = nhits[fragment_id][i]* mean*mean - 2 * mean*sum[fragment_id][i] + sumsq[fragment_id][i];
  double rms = sqrt(rms2/nhits[fragment_id][i]);
  // Send Metrics to the database **
  sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "Pedestal", mean, 0, artdaq::MetricMode::Average); 
  sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "RMS^2", rms2, 0, artdaq::MetricMode::Average); 
  sbndaq::sendMetric("CRT_channel", std::to_string(i + 32 * mac5), "RMS", rms, 0, artdaq::MetricMode::Average); 
 */

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

void sbndaq::BernCRTdqm::reconfigure(fhicl::ParameterSet const & pset)
{
  fBeamWindowStart = pset.get<int>("BeamWindowStart",320000);
  fBeamWindowEnd = pset.get<int>("BeamWindowEnd",350000);
} //reconfigure


DEFINE_ART_MODULE(sbndaq::BernCRTdqm)
//this is where the name is specified
