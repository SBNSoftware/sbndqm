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
  float fRMSCutWire;
  float fRMSCutRawDigit;
  int fNBADCutRawDigit;
  std::string fRawDigitLabel;

  int fNAwayFromPedestalRawDigit;
  int fDistFromPedestalRawDigit;

  size_t GetDigiNoiseChannels(const std::vector<raw::RawDigit>& rawdigits);
};


sunsetAna::TPCSunsetAnalyzer::TPCSunsetAnalyzer(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
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

  art::Handle digit_handle = e.getHandle<std::vector<raw::RawDigit>>("daq");
  if (digit_handle.isValid() && !digit_handle->empty()) {
    std::cout << "Found " << digit_handle->size() << " RawDigits." << std::endl;
    auto ndigi_noise = GetDigiNoiseChannels(*digit_handle);
    std::cout << "Found " << ndigi_noise << " channels with digital noise." << std::endl;
  }
  else{
     std::cout << "No RawDigits found in this event." << std::endl;
  }
}

size_t sunsetAna::TPCSunsetAnalyzer::GetDigiNoiseChannels(const std::vector<raw::RawDigit>& rawdigits){
  for (const auto& rd : rawdigits){
	  bool chanbad = false;
	  chanbad |= (rd.GetSigma() > fRMSCutRawDigit);
	  int nhexbad = 0;
	  std::vector<short> rawadc;
	  rawadc.resize(rd.Samples());
	  raw::Uncompress(rd.ADCs(), rawadc, rd.GetPedestal(), rd.Compression());
	  bool alleven = true;
	  int naway = 0;
	  const short adc0 = rawadc.at(0);
	  for (size_t i=0; i< rd.Samples(); ++i)
	    {
	      const short adc = rawadc.at(i);
	      if (adc == 0xBAD) ++nhexbad;
	      alleven &= ( ((adc - adc0) % 2) == 0 );
	      // if (std::abs(adc - rd.GetPedestal()) > fDistFromPedestalRawDigit){
		    //   ++naway;
        // }
	    }
	  chanbad |= (nhexbad > fNBADCutRawDigit);
	  chanbad |= alleven;
	  chanbad |= (naway > fNAwayFromPedestalRawDigit);
	  if (chanbad) fDNChannels.emplace(rd.Channel());
	}
  return fDNChannels.size();
}
DEFINE_ART_MODULE(sunsetAna::TPCSunsetAnalyzer)
