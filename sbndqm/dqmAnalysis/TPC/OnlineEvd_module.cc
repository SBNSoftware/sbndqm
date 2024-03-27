////////////////////////////////////////////////////////////////////////
// Class:       OnlineEvd
// Plugin Type: analyzer
// File:        OnlineEvd_module.cc
// Author:      tjyang@fnal.gov
// Analyzer module to make event display for DQM
////////////////////////////////////////////////////////////////////////
#include <vector>
#include <chrono>

#include "TROOT.h"
#include "TTree.h"
#include "TH2D.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TTimeStamp.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art_root_io/TFileService.h"

#include "ChannelData.hh"
#include "sbndqm/Decode/TPC/HeaderData.hh"
#include "Analysis.hh"

#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

namespace tpcAnalysis {
  class OnlineEvd;
}

class tpcAnalysis::OnlineEvd : public art::EDAnalyzer {
public:
  explicit OnlineEvd(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OnlineEvd(OnlineEvd const&) = delete;
  OnlineEvd(OnlineEvd&&) = delete;
  OnlineEvd& operator=(OnlineEvd const&) = delete;
  OnlineEvd& operator=(OnlineEvd&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

  // Selected optional functions.
  void beginJob() override;

private:
  void SendImages(const art::Event &e);
  // Declare member data here.
  std::vector<std::string> fRawDigitModuleLabels;
  std::string fRawDigitInstance;

  TH2D *hrawadc[6];
  int padx;
  int pady;
  int palette;
};


tpcAnalysis::OnlineEvd::OnlineEvd(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}
  , fRawDigitModuleLabels{p.get<std::vector<std::string>>("raw_digit_producers")}
  , fRawDigitInstance{p.get<std::string>("raw_digit_instance", "")}
  , padx{p.get<int>("padx", 1000)}
  , pady{p.get<int>("pady", 618)}
  , palette{p.get<int>("palette", 87)}
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void tpcAnalysis::OnlineEvd::SendImages(art::Event const& e)
{
  for (int i = 0; i<6; ++i){
    int tpc = i/3;
    int plane = i%3;
    // Read image file as binary data
    std::string image_path = Form("./sbnd_tpc%d_plane%d.png",tpc,plane);
    std::ifstream image_file(image_path, std::ios::binary);
    if (!image_file) {
      std::cerr << "Failed to open image file" << std::endl;
      return;
    }

    std::string image_data((std::istreambuf_iterator<char>(image_file)), std::istreambuf_iterator<char>());

    // Write image data to Redis
    std::string image_key = Form("tpc%d:plane%d:evd:image",tpc,plane);
    art::ServiceHandle<sbndaq::RedisConnectionService> redis;
    redis->Command("SET %s %b", image_key.c_str(), image_data.c_str(), image_data.size());
    //sbndaq::SendBinary(image_key, image_data.c_str(), image_data.size());
    sbndaq::SendEventMeta(image_key, e); 
  }
}

void tpcAnalysis::OnlineEvd::analyze(art::Event const& e)
{
  std::vector<art::Ptr<raw::RawDigit>> _raw_digits_handle;

  // Implementation of required member function here.

  int run = e.run();
  int event = e.id().event();
  art::Timestamp ts = e.time();
  TTimeStamp tts(ts.timeHigh(), ts.timeLow());
  
  //art::ServiceHandle<art::TFileService> tfs;

  int nbins[3] = {1984, 1984, 1664};
  for (int i = 0; i<6; ++i){
    int tpc = i/3;
    int plane = i%3;
    hrawadc[i] = new TH2D(Form("hrawadc%d",i),Form("Run %d Event %d TPC %d Plane %d;Wire;Tick;ADC", run, event, tpc, plane), nbins[plane], 0, nbins[plane]-1, 3400, 0, 3400);
    hrawadc[i]->SetMaximum(60);
    hrawadc[i]->SetMinimum(-20);
    hrawadc[i]->GetXaxis()->CenterTitle(true);
    hrawadc[i]->GetYaxis()->CenterTitle(true);
    hrawadc[i]->GetZaxis()->CenterTitle(true);
  }
  art::ServiceHandle<geo::Geometry> geo;

  // Get raw digits
  for (const std::string &prod: fRawDigitModuleLabels) {
    art::Handle<std::vector<raw::RawDigit>> digit_handle;
    if (fRawDigitInstance.size()) {
      e.getByLabel(prod, fRawDigitInstance, digit_handle);
    }
    else {
      e.getByLabel(prod, digit_handle);
    }

    // exit if the data isn't present
    if (!digit_handle.isValid()) {
      std::cerr << "Error: missing digits with producer (" << prod << ")" << std::endl;
      return;
    }
    art::fill_ptr_vector(_raw_digits_handle, digit_handle);
  }

  for (const auto & rd : _raw_digits_handle){

    auto adc_vec = rd->ADCs();
    int ch = rd->Channel();
    auto const & chids = geo->ChannelToWire(ch);
    int tpc = chids[0].TPC;
    int plane = chids[0].Plane;
    int wire = chids[0].Wire;
    //cout<<tpc<<" "<<plane<<" "<<wire<<endl;
    for (unsigned short i = 0; i<adc_vec.size(); ++i){
      hrawadc[plane + 3*tpc]->Fill(wire, i, adc_vec[i] - rd->GetPedestal());
    }
  }

  TLatex t;
  t.SetNDC();
  t.SetTextFont(132);
  t.SetTextSize(0.03);

  TCanvas *can = new TCanvas("can","can",padx,pady);
  for (int i = 0; i<6; ++i){
    int tpc = i/3;
    int plane = i%3;
    hrawadc[i]->Draw("colz");
    t.DrawLatex(0.01,0.01,Form("%d/%d/%d %d:%d:%d UTC",
			       tts.GetDate()/10000,
			       tts.GetDate()%10000/100,
			       tts.GetDate()%10000%100,
			       tts.GetTime()/10000,
			       tts.GetTime()%10000/100,
			       tts.GetTime()%10000%100));
    //can->Print(Form("sbnd_run%d_event%d_tpc%d_plane%d.png",run,event,tpc,plane));
    can->Print(Form("./sbnd_tpc%d_plane%d.png",tpc,plane));
    delete hrawadc[i];
  }
  
  delete can;

  SendImages(e);
}

void tpcAnalysis::OnlineEvd::beginJob()
{
  // Implementation of optional member function here.
  gStyle->SetOptStat(0);
  gStyle->SetPadBottomMargin(0.085);
  gStyle->SetPadTopMargin   (0.08);
  gStyle->SetPadLeftMargin  (0.08);
  gStyle->SetPadRightMargin (0.11);
  gStyle->SetLabelFont  ( 62   ,"XYZ");
  gStyle->SetTitleFont  ( 62   ,"XYZ");
  gStyle->SetTitleOffset( 1.15  , "x");
  gStyle->SetTitleOffset( 1.15  , "y");
  gStyle->SetTitleOffset( .7  , "z");
  gStyle->SetNumberContours(256);
  //gStyle->SetPalette(kRainBow);
  //gStyle->SetPalette(kLightTemperature);
  //gStyle->SetPalette(kGreenPink);
  gStyle->SetPalette(palette);
}

DEFINE_ART_MODULE(tpcAnalysis::OnlineEvd)
