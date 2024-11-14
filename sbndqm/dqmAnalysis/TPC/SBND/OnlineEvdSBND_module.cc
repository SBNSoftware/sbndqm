///////////////////////////////////////////////////////////////////////////
// Class:       OnlineEvdSBND
// Plugin Type: analyzer
// File:        OnlineEvdSBND_module.cc
// Author:      tjyang@fnal.gov
// Analyzer module to make event display for DQM
//
// April 22, 2024: Add two png files to show RMS in the WIB and FEM space
///////////////////////////////////////////////////////////////////////////
#include <vector>
#include <chrono>

#include "TROOT.h"
#include "TTree.h"
#include "TH2D.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TTimeStamp.h"
#include "TLine.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "larcore/Geometry/Geometry.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art_root_io/TFileService.h"

#include "sbndqm/dqmAnalysis/TPC/ChannelDataSBND.hh"
#include "sbndqm/Decode/TPC/HeaderData.hh"
#include "sbndqm/dqmAnalysis/TPC/AnalysisSBND.hh"

#include "sbndqm/Decode/TPC/SBND/DQMChannelMap/TPCDQMChannelMapService.h"
//#include "sbndcode/ChannelMaps/TPC/TPCChannelMapService.h"

#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include <vector>

using namespace std;

double calcrms(const vector<short> & adcs){

  if (adcs.empty()) return -100;
  double x = 0, x2 = 0;
  for (auto const & adc: adcs){
    x += adc;
    x2 += adc*adc;
  }
  return sqrt(x2/adcs.size() - pow(x/adcs.size(),2));
}

namespace tpcAnalysis {
  class OnlineEvdSBND;
}

class tpcAnalysis::OnlineEvdSBND : public art::EDAnalyzer {
public:
  explicit OnlineEvdSBND(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OnlineEvdSBND(OnlineEvdSBND const&) = delete;
  OnlineEvdSBND(OnlineEvdSBND&&) = delete;
  OnlineEvdSBND& operator=(OnlineEvdSBND const&) = delete;
  OnlineEvdSBND& operator=(OnlineEvdSBND&&) = delete;

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
  double qmax;
  double qmin;
  TH2D *wibplot;
  TH2D *femplot;
};


tpcAnalysis::OnlineEvdSBND::OnlineEvdSBND(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}
  , fRawDigitModuleLabels{p.get<std::vector<std::string>>("raw_digit_producers")}
  , fRawDigitInstance{p.get<std::string>("raw_digit_instance", "")}
  , padx{p.get<int>("padx", 1000*1.1)}
  , pady{p.get<int>("pady", 618*1.1)}
  , palette{p.get<int>("palette", 87)}
  , qmax{p.get<double>("qmax", 60)}
  , qmin{p.get<double>("qmin", -20)}
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void tpcAnalysis::OnlineEvdSBND::SendImages(art::Event const& e)
{
  // Send event display in channel vs time-tiack png files to Redis
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
  }

  // Send event display for WIBs and FEBs png files to Redis
  std::string wib_fem_file_paths[2] = {"./wibrms.png", "./femrms.png"};
  std::string wib_fem_redis_keys[2] = {"tpc:wibs:evd:image", "tpc:fems:evd:image"};
  for (int i = 0; i < 2; ++i){
    std::ifstream image_file(wib_fem_file_paths[i], std::ios::binary);
    if (!image_file) {
      std::cerr << "Failed to open image file" << std::endl;
      return;
    }

    std::string image_data((std::istreambuf_iterator<char>(image_file)), std::istreambuf_iterator<char>());

    art::ServiceHandle<sbndaq::RedisConnectionService> redis;
    redis->Command("SET %s %b", wib_fem_redis_keys[i].c_str(), image_data.c_str(), image_data.size());
  }
}

void tpcAnalysis::OnlineEvdSBND::analyze(art::Event const& e)
{
  std::vector<art::Ptr<raw::RawDigit>> _raw_digits_handle;

  // Implementation of required member function here.

  int run = e.run();
  int event = e.id().event();
  art::Timestamp ts = e.time();
  TTimeStamp tts(ts.timeHigh(), ts.timeLow());
  auto ttm = tts.GetSec();
  auto tlocal = std::localtime(&ttm);
  std::time_t ttt = std::mktime(tlocal);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&ttt), "%c %Z");
  //art::ServiceHandle<art::TFileService> tfs;

  int nbins[3] = {1984, 1984, 1664};
  for (int i = 0; i<6; ++i){
    int tpc = i/3;
    int plane = i%3;
    hrawadc[i] = new TH2D(Form("hrawadc%d",i),Form("Run %d Event %d TPC %d Plane %d;Wire;Tick;ADC", run, event, tpc, plane), nbins[plane], 0, nbins[plane]-1, 3420, 0, 3420);
    if (plane == 2){
         qmax = 200;
         qmin = -50;
    }
    else if (plane == 0) { 
        qmax = 80;
        qmin = -20;
    }
    else if (plane == 1) { 
        qmax = 80;
        qmin = -20;
    }
    hrawadc[i]->SetMaximum(qmax);
    hrawadc[i]->SetMinimum(qmin);
    hrawadc[i]->GetXaxis()->CenterTitle(true);
    hrawadc[i]->GetYaxis()->CenterTitle(true);
    hrawadc[i]->GetZaxis()->CenterTitle(true);
    for (int j = 1; j<=hrawadc[i]->GetNbinsX(); ++j){
      for (int k = 1; k<=hrawadc[i]->GetNbinsY(); ++k){
	hrawadc[i]->SetBinContent(j,k,0);
      }
    }
  }

  wibplot = new TH2D("wibplot", Form("Run %d Event %d;FEMB;WIB;RMS", run, event), 8*128, 0, 8*128, 12, 0, 12);
  wibplot->SetMaximum(10);
  wibplot->SetMinimum(-1);
  wibplot->GetXaxis()->CenterTitle(true);
  wibplot->GetYaxis()->CenterTitle(true);
  wibplot->GetZaxis()->CenterTitle(true);
  for (int i = 1; i<=wibplot->GetNbinsX(); ++i){
    for (int j = 1; j<=wibplot->GetNbinsY(); ++j){
      wibplot->SetBinContent(i, j, -100);
    }
  }

  femplot = new TH2D("femplot", Form("Run %d Event %d;FEM;TPC Crate;RMS", run, event), 16*64, 0, 16*64, 11, 0, 11);
  femplot->SetMaximum(10);
  femplot->SetMinimum(-1);
  femplot->GetXaxis()->CenterTitle(true);
  femplot->GetYaxis()->CenterTitle(true);
  femplot->GetZaxis()->CenterTitle(true);
  for (int i = 1; i<=femplot->GetNbinsX(); ++i){
    for (int j = 1; j<=femplot->GetNbinsY(); ++j){
      femplot->SetBinContent(i, j, -100);
    }
  }

  art::ServiceHandle<geo::Geometry> geo;
  art::ServiceHandle<SBND::TPCDQMChannelMapService> channelMap;

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
      //std::cout << "[tpcAnalysis::OnlineEvdSBND::analyze] adc_vec[" << i << "] : " << adc_vec[i] << ", rd->GetPedestal() : " << rd->GetPedestal() << std::endl;
      int bin = hrawadc[plane + 3*tpc]->GetBin(wire+0.1,i+0.1);
      double q = adc_vec[i] - rd->GetPedestal();
      if (plane == 2){
           qmax = 200;
           qmin = -50;
      }
      else if (plane == 0) { 
          qmax = 80;
          qmin = -20;
      }
      else if (plane == 1) { 
          qmax = 80;
          qmin = -20;
      }
      if (q>qmax) q = qmax;
      if (q<qmin) q = qmin;
      hrawadc[plane + 3*tpc]->SetBinContent(bin, q);
    }
    // Calculate rms
    double rms = calcrms(adc_vec);
    if (rms >10) rms = 10;
    auto const & chaninfo = channelMap->GetChanInfoFromOfflChan(ch);
    // Fill wib plot
    int binx = (chaninfo.WIBCrate-1)%2*128*4 +
      chaninfo.FEMBOnWIB * 128 +
      chaninfo.FEMBCh + 1;
    int biny = (1-(chaninfo.WIBCrate-1)/2)*6 +
      5 - (chaninfo.WIB-1) + 1;
    wibplot->SetBinContent(binx, biny, rms);
    // Fill fem plot
    binx = (chaninfo.FEM-1)*64 + chaninfo.FEMCh + 1;
    biny = 11-chaninfo.FEMCrate + 1;
    femplot->SetBinContent(binx, biny, rms);
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
//    t.DrawLatex(0.01,0.01,Form("%04d/%02d/%02d %02d:%02d:%02d %s",
//			       tlocal->tm_year+1900,
//			       tlocal->tm_mon+1,
//			       tlocal->tm_mday,
//			       tlocal->tm_hour,
//			       tlocal->tm_min,
//			       tlocal->tm_sec,
//			       tlocal->tm_isdst?"CDT":"CST"));
    t.DrawLatex(0.01,0.01,Form("%s",oss.str().c_str()));
    can->Print(Form("sbnd_tpc%d_plane%d.png",tpc,plane));
    //can->Print(Form("./sbnd_tpc%d_plane%d.pdf",tpc,plane));
    delete hrawadc[i];
  }

  wibplot->Draw("colz");
  wibplot->GetXaxis()->SetNdivisions(408,false);
  wibplot->GetYaxis()->SetNdivisions(112,false);
  wibplot->GetXaxis()->SetLabelSize(0);
  wibplot->GetYaxis()->SetLabelSize(0);
  TLine l1(0,6,128*8,6);
  l1.SetLineWidth(2);
  l1.Draw();
  TLine l2(128*4,0,128*4,12);
  l2.SetLineWidth(2);
  l2.Draw();

  TLine *l3[5];
  TLine *l4[5];
  for (int i = 0; i<5; ++i){
    l3[i] = new TLine(0, i+1, 128*8, i+1);
    l3[i]->SetLineWidth(1);
    l3[i]->Draw();
    l4[i] = new TLine(0, i+7, 128*8, i+7);
    l4[i]->SetLineWidth(1);
    l4[i]->Draw();
  }
  TLine *l5[8];
  for (int i = 0; i<8; ++i){
    if (i%4==3) continue;
    l5[i] = new TLine((i+1)*128,0,(i+1)*128,12);
    l5[i]->SetLineColor(0);
    l5[i]->SetLineStyle(2);
    l5[i]->SetLineWidth(1);
    l5[i]->Draw();
  }

  TLine *l6[32];
  for (int i = 0; i<32; ++i){
    if (i%4==3) continue;
    l6[i] = new TLine((i+1)*32,0,(i+1)*32,12);
    l6[i]->SetLineColor(0);
    l6[i]->SetLineStyle(3);
    l6[i]->SetLineWidth(1);
    l6[i]->Draw();
  }

  TLatex t2;
  //t2.SetTextFont(132);
  t2.SetTextSize(0.03);
  for (int i = 0; i<8; ++i){
    t2.DrawLatex(128*i+60,-0.4,Form("%d",i%4));
  }
  for (int i = 0; i<12; ++i){
    t2.DrawLatex(-30,i+0.4,Form("%d",6-i%6));
  }
  t2.DrawLatex(10,11.5,"SW1");
  t2.DrawLatex(10+128*4,11.5,"NW2");
  t2.DrawLatex(10,5.5,"SE3");
  t2.DrawLatex(10+128*4,5.5,"NE4");
  t.DrawLatex(0.01,0.01,Form("%s",oss.str().c_str()));
  can->Print("wibrms.png");

  femplot->Draw("colz");
  femplot->GetXaxis()->SetNdivisions(116,false);
  femplot->GetYaxis()->SetNdivisions(111,false);
  femplot->GetXaxis()->SetLabelSize(0);
  femplot->GetYaxis()->SetLabelSize(0);
  TLine *l7[11];
  for (int i = 0; i<11; ++i){
    l7[i] = new TLine(0, i+1, 16*64, i+1);
    l7[i]->SetLineWidth(1);
    l7[i]->Draw();
  }
  TLine *l8[15];
  for (int i = 0; i<15; ++i){
    l8[i] = new TLine((i+1)*64, 0, (i+1)*64, 11);
    l8[i]->SetLineWidth(1);
    l8[i]->SetLineColor(0);
    l8[i]->SetLineStyle(3);
    l8[i]->Draw();
  }
  for (int i = 0; i<16; ++i){
    t2.DrawLatex(64*i+28,-0.4,Form("%d",i+1));
  }
  for (int i = 0; i<11; ++i){
    t2.DrawLatex(-30,i+0.4,Form("%d",11-i));
  }
  t.DrawLatex(0.01,0.01,Form("%s",oss.str().c_str()));
  can->Print("femrms.png");

  delete wibplot;
  delete femplot;
  delete can;

  SendImages(e);
}

void tpcAnalysis::OnlineEvdSBND::beginJob()
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

DEFINE_ART_MODULE(tpcAnalysis::OnlineEvdSBND)
