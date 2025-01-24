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
//       Board-level:
//               MissingT0     - Total number of hits on this board in this event with missing T0 flag
//               MissingT1     - Total number of hits on this board in this event with missing T1 flag
//               ReadoutRate   - How many non-clock reset hits were there on this board in this event?
//               T0ClockDrift  - For T0 reset events, difference of T0 timestamp from exactly 1e9ns (1s)
//               Baseline      - Average pedestal across all 32 channels
//
//       Channel-level:
//               ChReadoutRate - How many non-clock reset hits were there on this board where this channel was the largest in this event?
//               pedestalMean  - Pedestal mean for a channel

// Current metrics being monitored:
//      Channel-level:
//              ADC          - the ADC value for a hit on a channel
//              lastbighit   - the time on a given hit since the last hit above 600 ADC threshold
//              ChFlag3Rate  - Number of flag 3 hit rate for each channel
//               pedestalRMS   - Pedestal RMS
//      Board-level:
//              T0            - T0 timestamp of a hit
//              T1            - T1 timestamp of a hit
//              T1Clockdrift  - For a T1 reset event, difference of T1 timestamp from beam signal (NEED TO IMPLEMENT)
//              Earlysynch    - Difference of timestamp to beginning of pull window
//              Latesynch     - Difference of timestamp to end of pull window
//              Deadtime      - time following any type of hit (of any flag) where the board cannot process another hit (time difference between consecutive hits  on the same board)
//
// To-do:
//      1. Make sure we handle different CRT walls with overlapping mac5 addresses properly
//      2. Turn lastbighit threshold into a fcl parameter
//      3. Include beam timing information to make T1Clockdrift useful
//
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/Common/BernCRTTranslator.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"

#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"

namespace sbndaq {
  class BernCRTdqmSBND;
}

class sbndaq::BernCRTdqmSBND : public art::EDAnalyzer {

public:
  explicit BernCRTdqmSBND(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~BernCRTdqmSBND();
  
  virtual void analyze(art::Event const & evt);
  void reconfigure(fhicl::ParameterSet const & pset);
 
private:

  //fhicl parameters
  bool fDebug;
  std::string fCRTModuleLabel;
  std::string fCRTInstanceLabel;
  int fBeamWindowStart;
  int fBeamWindowEnd;
  uint16_t fBigHitThreshold;
  std::vector<uint8_t> fMac5s;
};

sbndaq::BernCRTdqmSBND::BernCRTdqmSBND(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{
  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));

  this->reconfigure( pset );
}

sbndaq::BernCRTdqmSBND::~BernCRTdqmSBND()
{
}

void sbndaq::BernCRTdqmSBND::analyze(art::Event const &evt)
{
  if (fDebug) std::cout << "######################################################################" << std::endl;
  if (fDebug) std::cout << std::endl;
  if (fDebug) std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();
  
  art::Handle<std::vector<artdaq::Fragment>> fragmentHandle;
  evt.getByLabel(fCRTModuleLabel, fCRTInstanceLabel, fragmentHandle);

  if(!fragmentHandle.isValid() || fragmentHandle->size() == 0)
    return;

  std::vector<icarus::crt::BernCRTTranslator> hit_vector = icarus::crt::BernCRTTranslator::getCRTData(*fragmentHandle);
  if (fDebug) std::cout << "Successfully obtained CRT data" << std::endl;

  size_t num_t1_resets   = 0;
  size_t hitsperplane[7] = {0,0,0,0,0,0,0};

  // Initialize per module and per channel vars
  std::map<uint8_t, int> count_hit;
  std::map<uint8_t, uint64_t> deadtime;
  std::map<uint8_t, uint64_t> prev_fragment_timestamp;
  std::map<uint8_t, uint64_t> readoutRate;
  std::map<uint8_t, uint64_t> missingT0;
  std::map<uint8_t, uint64_t> missingT1;

  std::map<uint8_t, std::map<uint8_t, uint64_t>> lastBigHit;
  std::map<uint8_t, std::map<uint8_t, float>> chReadoutRate;

  for(const uint8_t& mac5 : fMac5s)
    {
      count_hit[mac5]               = 0;
      deadtime[mac5]                = std::numeric_limits<uint64_t>::max();
      prev_fragment_timestamp[mac5] = 0;
      readoutRate[mac5]             = 0;
      missingT0[mac5]               = 0;
      missingT1[mac5]               = 0;

      for(int ch = 0; ch < 32; ++ch)
	{
	  chReadoutRate[mac5][ch] = 0.;
	}
    }

  ///////////////////////////////////////
  // Extract Information from the Hits //
  ///////////////////////////////////////

  for(const auto & hit : hit_vector)
    {
      // Extract core hit information
      const uint8_t& mac5 = hit.mac5;
      const int ts0       = hit.ts0;
      const int ts1       = hit.ts1;
      const uint8_t flags = hit.flags;
      const uint16_t* adc = hit.adc;

      // Extract metadata information
      const uint16_t& fragment_id         = hit.fragment_ID;
      const uint64_t& fragment_timestamp  = hit.timestamp;
      const bool isTs0Reset               = hit.IsReference_TS0();
      const bool isTs1Reset               = hit.IsReference_TS1();
      const bool ts0Good                  = !hit.IsOverflow_TS0();
      const bool ts1Good                  = !hit.IsOverflow_TS1();
      const uint64_t& this_poll_end       = hit.this_poll_end;
      const uint64_t& last_poll_start     = hit.last_poll_start;

      std::string mac5_str = std::to_string(mac5);
      if (fDebug) std::cout << "Mac5: " << mac5_str <<std::endl;

      if(!ts0Good)
	++missingT0[mac5];

      if(!ts1Good)
	++missingT1[mac5];

      if(isTs0Reset && ts0Good)
	{
	  if (fDebug) std::cout << "Sending metric T0ClockDrift with value " << ts0 - static_cast<int>(1e9) << std::endl;
	  sbndaq::sendMetric("CRT_board", mac5_str, "T0ClockDrift", ts0 - static_cast<int>(1e9), 0, artdaq::MetricMode::LastPoint);
	}
    
      if(!isTs0Reset && !isTs1Reset && ts0Good)
	{
	  ++readoutRate[mac5];

	  uint16_t max_adc = 0;
	  uint8_t max_chan = 255;

	  for(uint8_t ch = 0; ch < 32; ch++)
	    {
	      if(adc[ch] > max_adc)
		{
		  max_adc = adc[ch];
		  max_chan = ch;
		}
	    }

	  ++chReadoutRate[mac5][max_chan];

	  uint8_t max_chan_pair = max_chan % 2 ? max_chan - 1 : max_chan + 1;

	  for(uint8_t ch = 0; ch < 32; ch++)
	    {
	      if(ch == max_chan || ch == max_chan_pair)
		continue;

	      if(adc[ch] > fBigHitThreshold)
		continue;

	      std::string ch_str = mac5_str + "_" + std::to_string(ch);
	      if (fDebug) std::cout << "Sending metric Pedestal with value " << adc[ch] << std::endl;
	      sbndaq::sendMetric("CRT_channel", ch_str, "Pedestal", adc[ch], 0, artdaq::MetricMode::Average);
	      sbndaq::sendMetric("CRT_board", mac5_str, "Baseline", adc[ch], 0, artdaq::MetricMode::Average);
	    }
	}
      else if(isTs0Reset || isTs1Reset)
	{
	  for(uint8_t ch = 0; ch < 32; ch++)
	    {
	      std::string ch_str = mac5_str + "_" + std::to_string(ch);
	      if (fDebug) std::cout << "Sending metric Pedestal with value " << adc[ch] << std::endl;
	      sbndaq::sendMetric("CRT_channel", ch_str, "Pedestal", adc[ch], 0, artdaq::MetricMode::Average);
	      sbndaq::sendMetric("CRT_board", mac5_str, "Baseline", adc[ch], 0, artdaq::MetricMode::Average);
	    }
	}

      for(int ch = 0; ch < 32; ch++)
	{
	  if(adc[ch] > fBigHitThreshold)
	    lastBigHit[mac5][ch] = fragment_timestamp;
	}

      // Deadtime
      if(count_hit[mac5] == 0)
	prev_fragment_timestamp[mac5] = fragment_timestamp;
      else
	{
	  uint64_t diff = fragment_timestamp - prev_fragment_timestamp[mac5];

	  if(diff < deadtime[mac5])
	    deadtime[mac5] = diff;

	  prev_fragment_timestamp[mac5] = fragment_timestamp;
	}

      ++count_hit[mac5];

      //      if(isTS1)
      //	num_t1_resets++;

      ///////////////////////////
      // Channel-Level Metrics //
      ///////////////////////////
    
      //  int baseline = (totaladc - max - secondmax)/30;

      uint64_t earlysynch = last_poll_start - fragment_timestamp;
      uint64_t latesynch  = fragment_timestamp - this_poll_end;
    
      auto thisone = fragment_id;  uint plane = (thisone & 0x0700) >> 8;
    
      if (fDebug) std::cout<<"Plane: "<<plane<<std::endl;
    
      if (plane>7) {if (fDebug) std::cout << "bad plane value " << plane << std::endl; plane=0;}
  
      // require that this is data and not clock reset (0xC), and that the ts1 time is valid (0x2)
      if (flags & 0x2 && !(flags & 0xC) ) {
	// check ts1 for beam window
	if(fDebug) std::cout<<"It's a data event! Ts1: "<<ts1<<std::endl;
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

      //      sbndaq::sendMetric("CRT_board", mac5_str, "MaxADCValue", max, 0, artdaq::MetricMode::LastPoint);
      //      sbndaq::sendMetric("CRT_board", mac5_str, "MaxADCChannel", maxindex + 32 * mac5, 0, artdaq::MetricMode::LastPoint);
      //      sbndaq::sendMetric("CRT_board", mac5_str, "Flag", flags, 0, artdaq::MetricMode::LastPoint);
      //      sbndaq::sendMetric("CRT_board", mac5_str, "baseline", baseline, 0, artdaq::MetricMode::Average);
      //      sbndaq::sendMetric("CRT_board", mac5_str, "TS0", ts0, 0, artdaq::MetricMode::LastPoint);
      //      sbndaq::sendMetric("CRT_board", mac5_str, "TS1", ts1, 0, artdaq::MetricMode::LastPoint);
      //      sbndaq::sendMetric("CRT_board", mac5_str, "Deadtime", deadtime, 0, artdaq::MetricMode::Minimum);
 
      //Sychronization Metrics
      sbndaq::sendMetric("CRT_board", mac5_str, "earlysynch", earlysynch, 0, artdaq::MetricMode::Average);
      sbndaq::sendMetric("CRT_board", mac5_str, "latesynch", latesynch, 0, artdaq::MetricMode::Average);

      // Flag 3 Hits (Board Level)

    } //loop over all CRT hits in an event
 
  for(const uint8_t& mac5 : fMac5s)
    {
      std::string mac5_str = std::to_string(mac5);

      if (fDebug) std::cout << "Sending metric MissingT0 with value " << missingT0[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "MissingT0", missingT0[mac5], 0, artdaq::MetricMode::Accumulate);

      if (fDebug) std::cout << "Sending metric MissingT1 with value " << missingT1[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "MissingT1", missingT1[mac5], 0, artdaq::MetricMode::Accumulate);

      if (fDebug) std::cout << "Sending metric ReadoutRate with value " << readoutRate[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "ReadoutRate", readoutRate[mac5], 0, artdaq::MetricMode::Rate);

      for(int ch = 0; ch < 32; ++ch)
	{
	  std::string ch_str = mac5_str + "_" + std::to_string(ch);
	  if (fDebug) std::cout << "Sending metric ChReadoutRate with value " << chReadoutRate[mac5][ch] << std::endl;
	  sbndaq::sendMetric("CRT_channel", ch_str, "ChReadoutRate", chReadoutRate[mac5][ch], 0, artdaq::MetricMode::Rate);
	}
    }

  /////////////////////////
  // Event-Level Metrics //
  /////////////////////////
  
  //"CRT hits in beam window per plane per event"
  for (int i=0;i<7;++i){
    if(fDebug) {std::cout<<"hitsperplane["<<i<<"]: "<<hitsperplane[i]<<std::endl;}
    sbndaq::sendMetric("CRT_event", std::to_string(0),
                       std::string("CRT_hits_beam_plane_")+std::to_string(i),
                       hitsperplane[i],
                       0, artdaq::MetricMode::LastPoint);
  }
  //"CRT T1 resets per event"
  if(fDebug) {std::cout<<"num_t1_resets: "<<num_t1_resets<<std::endl;}
  sbndaq::sendMetric("CRT_event", std::to_string(0),
                     "T1_resets_per_event",
                     num_t1_resets,
                     0, artdaq::MetricMode::LastPoint);
} //analyze

void sbndaq::BernCRTdqmSBND::reconfigure(fhicl::ParameterSet const & pset)
{
  fDebug            = pset.get<bool>("Debug", false);
  fCRTModuleLabel   = pset.get<std::string>("CRTModuleLabel", "daq");
  fCRTInstanceLabel = pset.get<std::string>("CRTInstanceLabel", "ContainerBERNCRTV2");
  fBeamWindowStart  = pset.get<int>("BeamWindowStart",320000);
  fBeamWindowEnd    = pset.get<int>("BeamWindowEnd",350000);
  fBigHitThreshold  = pset.get<uint16_t>("BigHitThreshold", 600);
  fMac5s            = pset.get<std::vector<uint8_t>>("metric_board_config.groups.CRT_board");
} //reconfigure

DEFINE_ART_MODULE(sbndaq::BernCRTdqmSBND)
