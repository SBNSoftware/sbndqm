///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
//
// This Module sends metrics from the SBND CRT modules to the redis database
// 
// Current metrics being monitored:
//       Board-level:
//               MissingT0      - Total number of hits on this board in this event with missing T0 flag
//               MissingT1      - Total number of hits on this board in this event with missing T1 flag
//               ReadoutRate    - How many non-clock reset hits were there on this board in this event?
//               T0ClockDrift   - For T0 reset events, difference of T0 timestamp from exactly 1e9ns (1s)
//               Baseline       - Average pedestal across all 32 channels
//               Deadtime       - Time difference between consecutive hits of any type (minimum value should be deadtime)
//               PullWindow     - Difference between first & last timestamp for that board in the event (maximum value should be the pull window)
//               NT0Resets      - Number of T0 reset events in this board in this event
//               NT1Resets      - Number of T1 reset events in this board in this event
//               T1ResetTDCDiff - Similar to T0ClockDrift, difference between the T0 timestamp of the T1 reset and the value recorded in the TDC
//
//       Channel-level:
//               ChReadoutRate  - How many non-clock reset hits were there on this board where this channel was the largest in this event?
//               Pedestal       - Pedestal mean for a channel
//               ADC            - Value of ADC when this channel is max (or paired with max)
//
//       Event-level:
//               T0ResetSpread  - The range between the lowest & highest T0 values for T0 reset events seen across all boards
//               T1ResetSpread  - The range between the lowest & highest T0 values for T1 reset events seen across all boards
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/RawEvent.hh"
#include "sbndaq-artdaq-core/Overlays/Common/BernCRTTranslator.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/TDCTimestampFragment.hh"

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
  uint64_t GetRawEventTime(art::Event const &evt);
  uint64_t GetSPECTDCT1ResetTime(art::Event const &evt, const uint64_t &raw_event_ts);
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
std::vector<uint8_t>       fMac5s;
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
  if (fDebug) std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event() << std::endl;

  uint64_t raw_event_ts = GetRawEventTime(evt);

  uint64_t tdc_t1_reset = GetSPECTDCT1ResetTime(evt, raw_event_ts);

  art::Handle<std::vector<artdaq::Fragment>> fragmentHandle;
  evt.getByLabel(fCRTModuleLabel, fCRTInstanceLabel, fragmentHandle);

  if(!fragmentHandle.isValid() || fragmentHandle->size() == 0)
    return;

  std::vector<icarus::crt::BernCRTTranslator> hit_vector = icarus::crt::BernCRTTranslator::getCRTData(*fragmentHandle);
  if (fDebug) std::cout << "Successfully obtained CRT data" << std::endl;

  // Initialize per module and per channel vars
  std::map<uint8_t, int> count_hit;
  std::map<uint8_t, uint64_t> prev_fragment_timestamp;
  std::map<uint8_t, uint16_t> readoutRate;
  std::map<uint8_t, uint16_t> missingT0;
  std::map<uint8_t, uint16_t> missingT1;
  std::map<uint8_t, uint64_t> min_timestamp;
  std::map<uint8_t, uint64_t> max_timestamp;
  std::map<uint8_t, uint16_t> nT0Resets;
  std::map<uint8_t, uint16_t> nT1Resets;
  std::map<uint8_t, uint32_t> t0Reset;
  std::map<uint8_t, uint32_t> t1Reset;

  std::map<uint8_t, std::map<uint8_t, float>> chReadoutRate;


  for(const uint8_t& mac5 : fMac5s)
    {
      count_hit[mac5]               = 0;
      prev_fragment_timestamp[mac5] = 0;
      readoutRate[mac5]             = 0;
      missingT0[mac5]               = 0;
      missingT1[mac5]               = 0;
      min_timestamp[mac5]           = std::numeric_limits<uint64_t>::max();
      max_timestamp[mac5]           = 0;
      nT0Resets[mac5]               = 0;
      nT1Resets[mac5]               = 0;
      t0Reset[mac5]                 = std::numeric_limits<uint32_t>::max();
      t1Reset[mac5]                 = std::numeric_limits<uint32_t>::max();

      for(int ch = 0; ch < 32; ++ch)
	chReadoutRate[mac5][ch] = 0.;
    }


  ///////////////////////////////////////
  // Extract Information from the Hits //
  ///////////////////////////////////////

  for(const auto & hit : hit_vector)
    {
      // Extract core hit information
      const uint8_t& mac5 = hit.mac5;
      const uint32_t ts0  = hit.ts0;
      const uint16_t* adc = hit.adc;

      // Extract metadata information
      const uint64_t& fragment_timestamp = hit.timestamp;
      const bool isTs0Reset              = hit.IsReference_TS0();
      const bool isTs1Reset              = hit.IsReference_TS1();
      const bool ts0Good                 = !hit.IsOverflow_TS0();
      const bool ts1Good                 = !hit.IsOverflow_TS1();

      std::string mac5_str = std::to_string(mac5);
      if (fDebug) std::cout << "Mac5: " << mac5_str <<std::endl;

      if(!ts0Good)
	++missingT0[mac5];

      if(!ts1Good)
	++missingT1[mac5];

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
		  uint32_t frac_tdc_t1_reset = tdc_t1_reset % static_cast<uint32_t>(1e9);
		  uint32_t diff              = ts0 > frac_tdc_t1_reset ? ts0 - frac_tdc_t1_reset : frac_tdc_t1_reset - ts0;
		  uint32_t curr_diff         = t1Reset[mac5] > frac_tdc_t1_reset ? t1Reset[mac5] - frac_tdc_t1_reset : frac_tdc_t1_reset - t1Reset[mac5];

		  if(diff < curr_diff)
		    t1Reset[mac5] = ts0;
		}
	      else
		t1Reset[mac5] = ts0;
	    }
	}

      if(isTs0Reset && ts0Good)
	{
	  const int t0ClockDrift = (int)ts0 - static_cast<int>(1e9);
	  if (fDebug) std::cout << "Sending metric T0ClockDrift with value " << t0ClockDrift << std::endl;
	  sbndaq::sendMetric("CRT_board", mac5_str, "T0ClockDrift", t0ClockDrift, 0, artdaq::MetricMode::LastPoint);
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
		  max_adc  = adc[ch];
		  max_chan = ch;
		}
	    }

	  ++chReadoutRate[mac5][max_chan];

	  uint8_t max_chan_pair = max_chan % 2 ? max_chan - 1 : max_chan + 1;

	  std::string max_chan_str      = mac5_str + "_" + std::to_string(max_chan);
	  std::string max_chan_pair_str = mac5_str + "_" + std::to_string(max_chan_pair);
	  if (fDebug) std::cout << "Sending metric ADC with values " << adc[max_chan] << " & " << adc[max_chan_pair] << std::endl;
	  sbndaq::sendMetric("CRT_channel", max_chan_str, "ADC", adc[max_chan], 0, artdaq::MetricMode::Average);
	  sbndaq::sendMetric("CRT_channel", max_chan_pair_str, "ADC", adc[max_chan_pair], 0, artdaq::MetricMode::Average);

	  for(uint8_t ch = 0; ch < 32; ch++)
	    {
	      if(ch == max_chan || ch == max_chan_pair)
		continue;

	      if(adc[ch] > fBigHitADCThreshold)
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

      if(count_hit[mac5] == 0)
	prev_fragment_timestamp[mac5] = fragment_timestamp;
      else
	{
	  uint64_t diff = fragment_timestamp - prev_fragment_timestamp[mac5];

	  if (fDebug) std::cout << "Sending metric Deadtime with value " << diff << std::endl;
	  sbndaq::sendMetric("CRT_board", mac5_str, "Deadtime", diff, 0, artdaq::MetricMode::Minimum);

	  prev_fragment_timestamp[mac5] = fragment_timestamp;
	}

      ++count_hit[mac5];

      if(fragment_timestamp < min_timestamp[mac5])
	min_timestamp[mac5] = fragment_timestamp;

      if(fragment_timestamp > max_timestamp[mac5])
	max_timestamp[mac5] = fragment_timestamp;
    } //loop over all CRT hits in an event


  ///////////////////////////////////////////////////////////////
  // (Produce and) send metrics that are calculated event long //
  ///////////////////////////////////////////////////////////////

  uint32_t t0_reset_min = std::numeric_limits<uint32_t>::max();
  uint32_t t0_reset_max = std::numeric_limits<uint32_t>::lowest();
  uint32_t t1_reset_min = std::numeric_limits<uint32_t>::max();
  uint32_t t1_reset_max = std::numeric_limits<uint32_t>::lowest();

  uint16_t boards_with_t0_reset = 0;
  uint16_t boards_with_t1_reset = 0;
 
  for(const uint8_t& mac5 : fMac5s)
    {
      std::string mac5_str = std::to_string(mac5);

      if (fDebug) std::cout << "Sending metric MissingT0 with value " << missingT0[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "MissingT0", missingT0[mac5], 0, artdaq::MetricMode::Accumulate);

      if (fDebug) std::cout << "Sending metric MissingT1 with value " << missingT1[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "MissingT1", missingT1[mac5], 0, artdaq::MetricMode::Accumulate);

      if (fDebug) std::cout << "Sending metric ReadoutRate with value " << readoutRate[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "ReadoutRate", readoutRate[mac5], 0, artdaq::MetricMode::Rate);

      if (fDebug) std::cout << "Sending metric PullWindow with value " << max_timestamp[mac5] - min_timestamp[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "PullWindow", max_timestamp[mac5] - min_timestamp[mac5], 0, artdaq::MetricMode::Maximum);

      if (fDebug) std::cout << "Sending metric NT0Resets with value " << nT0Resets[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "NT0Resets", nT0Resets[mac5], 0, artdaq::MetricMode::Maximum);

      if (fDebug) std::cout << "Sending metric NT1Resets with value " << nT1Resets[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5_str, "NT1Resets", nT1Resets[mac5], 0, artdaq::MetricMode::Maximum);

      if(nT0Resets[mac5] == 1 && t0Reset[mac5] != std::numeric_limits<uint32_t>::max())
	{
	  ++boards_with_t0_reset;

	  if(t0Reset[mac5] < t0_reset_min)
	    t0_reset_min = t0Reset[mac5];

	  if(t0Reset[mac5] > t0_reset_max)
	    t0_reset_max = t0Reset[mac5];
	}

      if(nT1Resets[mac5] == 1 && t1Reset[mac5] != std::numeric_limits<uint32_t>::max())
	{
	  ++boards_with_t1_reset;

	  if(t1Reset[mac5] < t1_reset_min)
	    t1_reset_min = t1Reset[mac5];

	  if(t1Reset[mac5] > t1_reset_max)
	    t1_reset_max = t1Reset[mac5];
	}

      if(tdc_t1_reset != std::numeric_limits<uint64_t>::max() && t1Reset[mac5] != std::numeric_limits<uint32_t>::max())
	{
	  uint32_t frac_tdc_t1_reset = tdc_t1_reset % static_cast<uint32_t>(1e9);
	  uint64_t t1ResetTDCDiff    = frac_tdc_t1_reset > t1Reset[mac5] ? frac_tdc_t1_reset - t1Reset[mac5] : t1Reset[mac5] - frac_tdc_t1_reset;

	  if (fDebug) std::cout << "Sending metric T1ResetTDCDiff with value " << t1ResetTDCDiff << std::endl;
	  sbndaq::sendMetric("CRT_board", mac5_str, "T1ResetTDCDiff", t1ResetTDCDiff, 0, artdaq::MetricMode::LastPoint);
	}

      for(int ch = 0; ch < 32; ++ch)
	{
	  std::string ch_str = mac5_str + "_" + std::to_string(ch);
	  if (fDebug) std::cout << "Sending metric ChReadoutRate with value " << chReadoutRate[mac5][ch] << std::endl;
	  sbndaq::sendMetric("CRT_channel", ch_str, "ChReadoutRate", chReadoutRate[mac5][ch], 0, artdaq::MetricMode::Rate);
	}
    }

  if(boards_with_t0_reset > fBoardsRequiredForResetSpread)
    {
      uint64_t t0ResetSpread = t0_reset_max - t0_reset_min;
      if (fDebug) std::cout << "Sending metric T0ResetSpread with value " << t0ResetSpread << std::endl;
      sbndaq::sendMetric("CRT_event", "0", "T0ResetSpread", t0ResetSpread, 0, artdaq::MetricMode::Maximum);
    }

  if(boards_with_t1_reset > fBoardsRequiredForResetSpread)
    {
      uint64_t t1ResetSpread = t1_reset_max - t1_reset_min;
      if (fDebug) std::cout << "Sending metric T1ResetSpread with value " << t1ResetSpread << std::endl;
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

uint64_t sbndaq::BernCRTdqmSBND::GetSPECTDCT1ResetTime(art::Event const &evt, const uint64_t &raw_event_ts)
{
  uint64_t min_diff  = std::numeric_limits<uint64_t>::max();
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
			  uint64_t diff = tdcTS->timestamp_ns() > raw_event_ts ? tdcTS->timestamp_ns() - raw_event_ts : raw_event_ts - tdcTS->timestamp_ns();

			  if(diff < min_diff)
			    {
			      min_diff  = diff;
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
		  timestamp = tdcTS->timestamp_ns();
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
  fCRTModuleLabel               = pset.get<std::string>("CRTModuleLabel", "daq");
  fCRTInstanceLabel             = pset.get<std::string>("CRTInstanceLabel", "ContainerBERNCRTV2");
  fSPECTDCModuleLabel           = pset.get<std::string>("SPECTDCModuleLabel", "daq");
  fSPECTDCInstanceLabels        = pset.get<std::vector<std::string>>("SPECTDCInstanceLabels", {"TDCTIMESTAMP", "ContainerTDCTIMESTAMP"});
  fDAQHeaderModuleLabel         = pset.get<std::string>("DAQHeaderModuleLabel", "");
  fDAQHeaderInstanceLabel       = pset.get<std::string>("DAQHeaderInstanceLabel", "");
  fSPECTDCT1Channel             = pset.get<uint16_t>("SPECTDCT1Channel", 0);
  fRawTSCorrection              = pset.get<uint64_t>("RawTSCorrection", 367000);
  fBigHitADCThreshold           = pset.get<uint16_t>("BigHitADCThreshold", 600);
  fBoardsRequiredForResetSpread = pset.get<uint16_t>("BoardsRequiredForResetSpread", 100);
  fMac5s                        = pset.get<std::vector<uint8_t>>("metric_board_config.groups.CRT_board");
} //reconfigure

DEFINE_ART_MODULE(sbndaq::BernCRTdqmSBND)
