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
  std::vector<uint8_t>     fMac5s;
};

sbndaq::BernCRTdqmSBND::BernCRTdqmSBND(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{
  if(pset.has_key("metrics"))
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));

  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_event_config"));

  this->reconfigure(pset);
}

sbndaq::BernCRTdqmSBND::~BernCRTdqmSBND()
{
}

void sbndaq::BernCRTdqmSBND::analyze(art::Event const &evt)
{
  if(fDebug) std::cout << "######################################################################" << std::endl;
  if(fDebug) std::cout << std::endl;
  if(fDebug) std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event() << std::endl;

  uint64_t rawEventTS = GetRawEventTime(evt);

  uint64_t tdcT1Reset = GetSPECTDCT1ResetTime(evt, rawEventTS);

  art::Handle<std::vector<artdaq::Fragment>> fragmentHandle;
  evt.getByLabel(fCRTModuleLabel, fCRTInstanceLabel, fragmentHandle);

  if(!fragmentHandle.isValid() || fragmentHandle->size() == 0)
    return;

  std::vector<icarus::crt::BernCRTTranslator> hitVector = icarus::crt::BernCRTTranslator::getCRTData(*fragmentHandle);
  if(fDebug) std::cout << "Successfully obtained CRT data" << std::endl;

  // Initialize per module and per channel vars
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


  ///////////////////////////////////////
  // Extract Information from the Hits //
  ///////////////////////////////////////

  for(const auto & hit : hitVector)
    {
      // Extract core hit information
      const uint8_t& mac5 = hit.mac5;
      const uint32_t ts0  = hit.ts0;
      const uint16_t* adc = hit.adc;

      // Extract metadata information
      const uint64_t& fragmentTS = hit.timestamp;
      const bool isTs0Reset      = hit.IsReference_TS0();
      const bool isTs1Reset      = hit.IsReference_TS1();
      const bool ts0Good         = !hit.IsOverflow_TS0();
      const bool ts1Good         = !hit.IsOverflow_TS1();

      std::string mac5Str = std::to_string(mac5);
      if(fDebug) std::cout << "Mac5: " << mac5Str <<std::endl;

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

      if(isTs0Reset && ts0Good)
        {
          const int t0ClockDrift = (int)ts0 - static_cast<int>(1e9);
          if(fDebug) std::cout << "Sending metric T0ClockDrift with value " << t0ClockDrift << std::endl;
          sbndaq::sendMetric("CRT_board", mac5Str, "T0ClockDrift", t0ClockDrift, 0, artdaq::MetricMode::LastPoint);
        }
    
      if(!isTs0Reset && !isTs1Reset && ts0Good)
        {
          ++readoutRate[mac5];

          uint16_t maxADC = 0;
          uint8_t maxChan = 255;

          for(uint8_t ch = 0; ch < 32; ch++)
            {
              if(adc[ch] > maxADC)
                {
                  maxADC  = adc[ch];
                  maxChan = ch;
                }
            }

          ++chReadoutRate[mac5][maxChan];

          uint8_t maxChanPair = maxChan % 2 ? maxChan - 1 : maxChan + 1;

          std::string maxChanStr     = mac5Str + "_" + std::to_string(maxChan);
          std::string maxChanPairStr = mac5Str + "_" + std::to_string(maxChanPair);
          if(fDebug) std::cout << "Sending metric ADC with values " << adc[maxChan] << " & " << adc[maxChanPair] << std::endl;
          sbndaq::sendMetric("CRT_channel", maxChanStr, "ADC", adc[maxChan], 0, artdaq::MetricMode::Average);
          sbndaq::sendMetric("CRT_channel", maxChanPairStr, "ADC", adc[maxChanPair], 0, artdaq::MetricMode::Average);

          for(uint8_t ch = 0; ch < 32; ch++)
            {
              if(ch == maxChan || ch == maxChanPair)
                continue;

              if(adc[ch] > fBigHitADCThreshold)
                continue;

              std::string chStr = std::to_string(mac5*100 + ch);
              if(fDebug) std::cout << "Sending metric Pedestal with value " << adc[ch] << std::endl;
              sbndaq::sendMetric("CRT_channel", chStr, "Pedestal", adc[ch], 0, artdaq::MetricMode::Average);
              sbndaq::sendMetric("CRT_board", mac5Str, "Baseline", adc[ch], 0, artdaq::MetricMode::Average);
            }
        }
      else if(isTs0Reset || isTs1Reset)
        {
          for(uint8_t ch = 0; ch < 32; ch++)
            {
              std::string chStr = std::to_string(mac5*100 + ch);
              if(fDebug) std::cout << "Sending metric Pedestal with value " << adc[ch] << std::endl;
              sbndaq::sendMetric("CRT_channel", chStr, "Pedestal", adc[ch], 0, artdaq::MetricMode::Average);
              sbndaq::sendMetric("CRT_board", mac5Str, "Baseline", adc[ch], 0, artdaq::MetricMode::Average);
            }
        }

      if(hitCount[mac5] == 0)
        prevFragmentTS[mac5] = fragmentTS;
      else
        {
          uint64_t diff = fragmentTS - prevFragmentTS[mac5];

          if(fDebug) std::cout << "Sending metric Deadtime with value " << diff << std::endl;
          sbndaq::sendMetric("CRT_board", mac5Str, "Deadtime", diff, 0, artdaq::MetricMode::Minimum);

          prevFragmentTS[mac5] = fragmentTS;
        }

      ++hitCount[mac5];

      if(fragmentTS < minTS[mac5])
        minTS[mac5] = fragmentTS;

      if(fragmentTS > maxTS[mac5])
        maxTS[mac5] = fragmentTS;
    } //loop over all CRT hits in an event


  ///////////////////////////////////////////////////////////////
  // (Produce and) send metrics that are calculated event long //
  ///////////////////////////////////////////////////////////////

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
      sbndaq::sendMetric("CRT_board", mac5Str, "MissingT0", missingT0[mac5], 0, artdaq::MetricMode::Accumulate);

      if(fDebug) std::cout << "Sending metric MissingT1 with value " << missingT1[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5Str, "MissingT1", missingT1[mac5], 0, artdaq::MetricMode::Accumulate);

      if(fDebug) std::cout << "Sending metric ReadoutRate with value " << readoutRate[mac5] << std::endl;
      sbndaq::sendMetric("CRT_board", mac5Str, "ReadoutRate", readoutRate[mac5], 0, artdaq::MetricMode::Rate);

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
          sbndaq::sendMetric("CRT_board", mac5Str, "T1ResetTDCDiff", t1ResetTDCDiff, 0, artdaq::MetricMode::LastPoint);
        }

      for(int ch = 0; ch < 32; ++ch)
        {
          std::string chStr = std::to_string(mac5*100 + ch);
          if(fDebug) std::cout << "Sending metric ChReadoutRate with value " << chReadoutRate[mac5][ch] << std::endl;
          sbndaq::sendMetric("CRT_channel", chStr, "ChReadoutRate", chReadoutRate[mac5][ch], 0, artdaq::MetricMode::Rate);
        }
    }

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
  fMac5s                        = pset.get<std::vector<uint8_t>>("metric_board_config.groups.CRT_board");
} //reconfigure

DEFINE_ART_MODULE(sbndaq::BernCRTdqmSBND)
