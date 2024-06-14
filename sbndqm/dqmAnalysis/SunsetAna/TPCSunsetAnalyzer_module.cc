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

#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/raw.h"

#include "sbndqm/Decode/Mode/Mode.hh"

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

  int fdefaultIndBaseline;
  int fdefaultColBaseline;

  float fRMSCutWire;
  float fRMSCutRawDigit;
  int fNBADCutRawDigit;
  std::string fRawDigitLabel;

  int fNAwayFromPedestalRawDigit;
  int fDistFromPedestalRawDigit;

  bool   isSunsetEvent(const std::vector<raw::RawDigit>& rawdigits);
  void   getBaselines(const std::vector<raw::RawDigit>& rawdigits, std::vector<float>& baselines);
  size_t getDigiNoiseChannels(const std::vector<raw::RawDigit>& rawdigits);
};


sunsetAna::TPCSunsetAnalyzer::TPCSunsetAnalyzer(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  fdefaultIndBaseline = p.get<int>("defaultIndBaseline",2048);
  fdefaultColBaseline = p.get<int>("defaultColBaseline",460);
  // Call appropriate consumes<>() for any products to be retrieved by this module.
  fRMSCutWire = p.get<float>("RMSCutWire",100.0);
  fRMSCutRawDigit = p.get<float>("RMSCutRawDigit",100.0);
  fNBADCutRawDigit = p.get<float>("NBADCutRawDigit",5);
  fRawDigitLabel = p.get<std::string>("RawDigitLabel","daq");
  fNAwayFromPedestalRawDigit = p.get<int>("NAwayFromPedestalRawDigit",100);
  fDistFromPedestalRawDigit = p.get<int>("DistFromPedestalRawDigit",100);
}

void sunsetAna::TPCSunsetAnalyzer::analyze(art::Event const& e)
{
  fDNChannels.clear();
  if (fBaselines.empty()){
    fBaselines.resize(11264);
    std::fill(fBaselines.begin(), fBaselines.begin()+3968, fdefaultIndBaseline);
    std::fill(fBaselines.begin()+3968, fBaselines.begin()+5632, fdefaultColBaseline);
    std::fill(fBaselines.begin()+5632, fBaselines.begin()+9000, fdefaultIndBaseline);
    std::fill(fBaselines.begin()+9000, fBaselines.end(), fdefaultColBaseline);
  }

  art::Handle digit_handle = e.getHandle<std::vector<raw::RawDigit>>("daq");
  if (digit_handle.isValid() && !digit_handle->empty()) {
    std::cout << "Found " << digit_handle->size() << " RawDigits." << std::endl;
    auto ndigi_noise = getDigiNoiseChannels(*digit_handle);
    std::cout << "Found " << ndigi_noise << " channels with digital noise." << std::endl;
    bool isSunset = isSunsetEvent(*digit_handle);
    if (isSunset==false){
      getBaselines(*digit_handle, fBaselines);
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
  if (avg_maxval > 0)
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
        // }
        if (i < rd.Samples()-1){
          const short adc_next = rawadc.at(i+1);
          if (i < 500)     pre_neighbor_diff += std::abs(adc - adc_next);
          else if (i>1000) post_neighbor_diff += std::abs(adc - adc_next);
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
    std::vector<short> rawadc;
    rawadc.resize(rd.Samples());
    raw::Uncompress(rd.ADCs(), rawadc, rd.GetPedestal(), rd.Compression());
    baselines.at(chnum) = Mode(rawadc, 10);
  }
}

DEFINE_ART_MODULE(sunsetAna::TPCSunsetAnalyzer)
