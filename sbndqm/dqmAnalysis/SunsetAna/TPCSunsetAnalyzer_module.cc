////////////////////////////////////////////////////////////////////////
// Class:       TPCSunsetAnalyzer
// Plugin Type: analyzer (Unknown Unknown)
// File:        TPCSunsetAnalyzer_module.cc
//
// Generated at Thu Jun 13 16:50:27 2024 by Lynn Tung using cetskelgen
// from  version .
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art_root_io/TFileService.h"

#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/raw.h"

#include "sbndqm/Decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include "TFile.h"
#include "TTree.h"

#include <unordered_set>

namespace sunsetAna {
  class TPCSunsetAnalyzer;
}


class sunsetAna::TPCSunsetAnalyzer : public art::EDAnalyzer {
public:
  explicit TPCSunsetAnalyzer(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  TPCSunsetAnalyzer(TPCSunsetAnalyzer const&) = delete;
  TPCSunsetAnalyzer(TPCSunsetAnalyzer&&) = delete;
  TPCSunsetAnalyzer& operator=(TPCSunsetAnalyzer const&) = delete;
  TPCSunsetAnalyzer& operator=(TPCSunsetAnalyzer&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

private:
  std::unordered_set<raw::ChannelID_t> fDNChannels;   // set of channels with digital noise on them on this event
  std::vector<float> fBaselines; // storage for baselines, needed to reference during sunset events
  std::vector<int>   fSpikeChannels; // set of channels that are spiked
  std::vector<int>   fSpikeEndTicks;  // the tick value for when a spike ends

  std::string fRawDigitLabel;

  bool foffline = false;
  bool fuseMedian; // if False, use mode

  int fdefaultIndBaseline;
  int fdefaultColBaseline;

  float fsunset_tick;  // the tick value where we expect the sunset to start! needs to be correct within ~10 ticks
  float fblob_ADCsum_thresh; // the metric for which we identify whether an event is a sunset or not

  std::vector<float> fspike_tick_vals; // the tick value to look for spikes, AFTER the tick value where the sunset happens
  std::vector<float> fspike_adc_thresh; // the threshold to determine if a spike is present

  float fRMSCutWire;
  float fRMSCutRawDigit;
  int fNBADCutRawDigit;

  int fNAwayFromPedestalRawDigit;
  int fDistFromPedestalRawDigit;

  bool   isSunsetEvent(const std::vector<raw::RawDigit>& rawdigits);
  void   getBaselines(const std::vector<raw::RawDigit>& rawdigits, std::vector<float>& baselines);
  size_t getDigiNoiseChannels(const std::vector<raw::RawDigit>& rawdigits);
  void   getSpikes(const std::vector<raw::RawDigit>& rawdigits);
  short  Median(std::vector<short> wvfm);

  TTree *_tree;
  int _run, _subrun, _event;
  bool _isSunset;
  int _nspikes, _nspikes_long;
};


sunsetAna::TPCSunsetAnalyzer::TPCSunsetAnalyzer(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  fRawDigitLabel = p.get<std::string>("RawDigitLabel","daq");
  foffline = p.get<bool>("offline",false);

  fuseMedian = p.get<bool>("useMedian",true);
  fdefaultIndBaseline = p.get<int>("defaultIndBaseline",2048);
  fdefaultColBaseline = p.get<int>("defaultColBaseline",460);

  fsunset_tick = p.get<float>("sunset_tick",500);
  fblob_ADCsum_thresh = p.get<float>("blob_ADCsum_thresh",1000);
  fspike_tick_vals = p.get<std::vector<float>>("spike_tick_vals",{500,250,250});
  fspike_adc_thresh = p.get<std::vector<float>>("spike_adc_thresh",{150,150,125});

  fRMSCutWire = p.get<float>("RMSCutWire",100.0);
  fRMSCutRawDigit = p.get<float>("RMSCutRawDigit",100.0);
  fNBADCutRawDigit = p.get<float>("NBADCutRawDigit",5);
  fNAwayFromPedestalRawDigit = p.get<int>("NAwayFromPedestalRawDigit",100);
  fDistFromPedestalRawDigit = p.get<int>("DistFromPedestalRawDigit",100);

  art::ServiceHandle<art::TFileService> fs;
  if (foffline){
    _tree = fs->make<TTree>("sunset_tpc_tree","");
    _tree->Branch("run", &_run, "run/I");
    _tree->Branch("subrun", &_subrun, "subrun/I");
    _tree->Branch("event", &_event, "event/I");
    _tree->Branch("isSunset", &_isSunset, "isSunset/O");
    _tree->Branch("nspikes", &_nspikes, "nspikes/I");
    _tree->Branch("nspikes_long", &_nspikes_long, "nspikes_long/I");
  }

  if (p.has_key("metrics"))
    sbndaq::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
  sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_sunsets"));
}

void sunsetAna::TPCSunsetAnalyzer::analyze(art::Event const& e)
{
  fDNChannels.clear();
  fSpikeChannels.clear();
  fSpikeEndTicks.clear();
  auto level = 3;

  if (fBaselines.empty()){
    fBaselines.resize(11264);
    std::fill(fBaselines.begin(), fBaselines.begin()+3968, fdefaultIndBaseline);
    std::fill(fBaselines.begin()+3968, fBaselines.begin()+5632, fdefaultColBaseline);
    std::fill(fBaselines.begin()+5632, fBaselines.begin()+9000, fdefaultIndBaseline);
    std::fill(fBaselines.begin()+9000, fBaselines.end(), fdefaultColBaseline);
  }

  art::Handle digit_handle = e.getHandle<std::vector<raw::RawDigit>>(fRawDigitLabel);
  if (digit_handle.isValid() && !digit_handle->empty()) {
    std::cout << "Found " << digit_handle->size() << " RawDigits." << std::endl;
    auto ndigi_noise = getDigiNoiseChannels(*digit_handle);
    std::cout << "Found " << ndigi_noise << " channels with digital noise." << std::endl;
    sbndaq::sendMetric("ndigi", ndigi_noise, level, artdaq::MetricMode::LastPoint);

    bool isSunset = isSunsetEvent(*digit_handle);
    int nspikes = 0;
    int nspikes_long = 0;
    if (isSunset==false){
      getBaselines(*digit_handle, fBaselines);
    }
    else{
      // is sunset... time to extract metrics!
      getSpikes(*digit_handle);
      nspikes = fSpikeChannels.size();
      for (size_t i=0; i < size_t(nspikes); i++){
        // std::cout << fSpikeEndTicks.at(i) << std::endl;
        int readout_length = digit_handle->at(0).Samples();
        // std::cout << "readout length" << readout_length << std::endl;
        if (fSpikeEndTicks.at(i) == readout_length){
          nspikes_long++;
        }
      }
      std::cout << "Found " << fSpikeChannels.size() << " channels with spikes." << std::endl;    
      std::cout << "Found " << nspikes_long << " channels with spikes that exceed the readout window." << std::endl;
      sbndaq::sendMetric("nspikes", nspikes, level, artdaq::MetricMode::LastPoint);

    }
    if (foffline){
      _run = e.run();
      _subrun = e.subRun();
      _event = e.event();
      _isSunset = isSunset;
      _nspikes = nspikes;
      _nspikes_long = nspikes_long;
      _tree->Fill();
    }
  }
  else{
     std::cout << "No RawDigits found in this event." << std::endl;
  }
}

bool sunsetAna::TPCSunsetAnalyzer::isSunsetEvent(const std::vector<raw::RawDigit>& rawdigits){
  // look in the blob region, choose 6600:6700
  // get the average max value in this region
  float avg_maxval = 0;
  auto nblob_ch = 0;

  for (const auto& rd : rawdigits){
    uint blob_min_ch = 6600;
    uint blob_max_ch = 6700; 
    auto chnum = rd.Channel();

    if (chnum > blob_min_ch && chnum < blob_max_ch){
      // ignore if this happens to be a digital noise channel
      if (fDNChannels.find(chnum) != fDNChannels.end()) continue;

      std::vector<short> rawadc;
      rawadc.resize(rd.Samples());
	    raw::Uncompress(rd.ADCs(), rawadc, rd.GetPedestal(), rd.Compression());
      // get the max value in this region
      short maxval = *std::max_element(rawadc.begin(), rawadc.end()) - fBaselines.at(chnum);
      avg_maxval += maxval;
      nblob_ch++;
    }
  }
  if (nblob_ch) avg_maxval /= nblob_ch;
  std::cout << "Average max value in blob region: " << avg_maxval << std::endl;
  if (avg_maxval > fblob_ADCsum_thresh)
    return true;
  else
    return false;
}

size_t sunsetAna::TPCSunsetAnalyzer::getDigiNoiseChannels(const std::vector<raw::RawDigit>& rawdigits){
  for (const auto& rd : rawdigits){
	  bool chanbad = false;
	  chanbad |= (rd.GetSigma() > fRMSCutRawDigit);
	  int nhexbad = 0;
	  std::vector<short> rawadc;
	  rawadc.resize(rd.Samples());
	  raw::Uncompress(rd.ADCs(), rawadc, rd.GetPedestal(), rd.Compression());
	  bool alleven = true;
	  int naway = 0;
    int pre_neighbor_diff = 0;
    int post_neighbor_diff = 0;
	  const short adc0 = rawadc.at(0);
    // auto chnum = rd.Channel();
	  for (size_t i=0; i< rd.Samples(); ++i)
	    {
	      const short adc = rawadc.at(i);
	      if (adc == 0xBAD) ++nhexbad;
	      alleven &= ( ((adc - adc0) % 2) == 0 );
        // ** this will make any sunset channels digital noise channels **
	      // if (std::abs(adc - fBaselines.at(chnum)) > fDistFromPedestalRawDigit){
		      // ++naway;
        // }ss

        // calculates the difference between neighboring channels 
        if (i < rd.Samples()-1){
          const short adc_next = rawadc.at(i+1);
          if (i < fsunset_tick)     pre_neighbor_diff += std::abs(adc - adc_next);
          else if (i>fsunset_tick+500) post_neighbor_diff += std::abs(adc - adc_next);
        }
      }
	  chanbad |= (nhexbad > fNBADCutRawDigit);
	  chanbad |= alleven;
	  chanbad |= (naway > fNAwayFromPedestalRawDigit);
    chanbad |= (pre_neighbor_diff > 5e4);
    chanbad |= (post_neighbor_diff > 5e4);
	  if (chanbad) fDNChannels.emplace(rd.Channel());
	}
  return fDNChannels.size();
}

void sunsetAna::TPCSunsetAnalyzer::getBaselines(const std::vector<raw::RawDigit>& rawdigits, std::vector<float>& baselines){
  for (const auto& rd : rawdigits){
    auto chnum = rd.Channel();
    if (fDNChannels.find(chnum) != fDNChannels.end()) continue;
    std::vector<short> rawadc;
    rawadc.resize(rd.Samples());
    raw::Uncompress(rd.ADCs(), rawadc, rd.GetPedestal(), rd.Compression());
    if (fuseMedian) baselines.at(chnum) = Median(rawadc);
    else baselines.at(chnum) = Mode(rawadc, 10);
  }
}

short sunsetAna::TPCSunsetAnalyzer::Median(std::vector<short> wvfm){
  const auto median_it = wvfm.begin() + wvfm.size() / 2;
  std::nth_element(wvfm.begin(), median_it , wvfm.end());
  auto median = *median_it;
  return median;
}

void sunsetAna::TPCSunsetAnalyzer::getSpikes(const std::vector<raw::RawDigit>& rawdigits){
  for (const auto& rd : rawdigits){
    auto chnum = rd.Channel();
    if (fDNChannels.find(chnum) != fDNChannels.end()) continue;

    std::vector<short> rawadc;
    rawadc.resize(rd.Samples());
    raw::Uncompress(rd.ADCs(), rawadc, rd.GetPedestal(), rd.Compression());
    
    int plane; 
    if (chnum < 1984 || ((chnum < 7616) && (chnum>5632))) plane = 0;
    else if ((chnum >= 1984 && chnum < 3968) || (chnum >= 7616 && chnum < 9000)) plane = 1;
    else plane = 2;

    auto this_spike_threshold = fspike_adc_thresh.at(plane) + fBaselines.at(chnum);
    auto this_spike_tick = fspike_tick_vals.at(plane) + fsunset_tick; 
    bool is_spike = rawadc.at(this_spike_tick) > this_spike_threshold;
    if (is_spike){
      fSpikeChannels.push_back(chnum);
      for (size_t i=this_spike_tick; i< rd.Samples()-1; ++i){
        short neighbor_diff = rawadc.at(i+1) - rawadc.at(i);
        if (neighbor_diff < -1e1){
          // the shelf ends d
          fSpikeEndTicks.push_back(i);
          break;
        }
        else if (i==rd.Samples()-2){
          // if you reach the end and shelf hasn't ended...
          fSpikeEndTicks.push_back(rd.Samples());
        }
      }
    }
  }
}

DEFINE_ART_MODULE(sunsetAna::TPCSunsetAnalyzer)
