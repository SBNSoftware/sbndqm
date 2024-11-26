///////////////////////////////////////////////////////////////////////////
// Class:       OfflineEvdSBND
// Plugin Type: analyzer
// File:        OfflineEvdSBND_module.cc
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
  class OfflineEvdSBND;
}

class tpcAnalysis::OfflineEvdSBND : public art::EDAnalyzer {
public:
  explicit OfflineEvdSBND(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OfflineEvdSBND(OfflineEvdSBND const&) = delete;
  OfflineEvdSBND(OfflineEvdSBND&&) = delete;
  OfflineEvdSBND& operator=(OfflineEvdSBND const&) = delete;
  OfflineEvdSBND& operator=(OfflineEvdSBND&&) = delete;

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
  double qmax_0;
  double qmax_1;
  double qmax_2;
  double qmin_0;
  double qmin_1;
  double qmin_2;
};


tpcAnalysis::OfflineEvdSBND::OfflineEvdSBND(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}
  , fRawDigitModuleLabels{p.get<std::vector<std::string>>("raw_digit_producers")}
  , fRawDigitInstance{p.get<std::string>("raw_digit_instance", "")}
  , padx{p.get<int>("padx", 1000*1.1)}
  , pady{p.get<int>("pady", 618*1.1)}
  , palette{p.get<int>("palette", 87)}
  , qmax_0{p.get<double>("qmax_0", 80)}
  , qmax_1{p.get<double>("qmax_1", 80)}
  , qmax_2{p.get<double>("qmax_2", 200)}
  , qmin_0{p.get<double>("qmin_0", -20)}
  , qmin_1{p.get<double>("qmin_1", -20)}
  , qmin_2{p.get<double>("qmin_2", -50)}
{
  // Call appropriate consumes<>() for any products to be retrieved by this module.
}

void tpcAnalysis::OfflineEvdSBND::SendImages(art::Event const& e)
{

  int run = e.run();
  int subrun = e.subRun();
  int event = e.id().event();
  // Send event display in channel vs time-tiack png files to Redis
  for (int i = 0; i<6; ++i){
    int tpc = i/3;
    int plane = i%3;
    // Read image file as binary data
    std::string image_path = Form("/daq/log/TPC_EVD/sbnd_run%d_subrun%d_event%d_tpc%d_plane%d.png",run,subrun, event, tpc,plane);
    std::ifstream image_file(image_path, std::ios::binary);
    if (!image_file) {
      std::cerr << "Failed to open image file" << std::endl;
      return;
    }
    std::string image_data((std::istreambuf_iterator<char>(image_file)), std::istreambuf_iterator<char>());
  }
}

void tpcAnalysis::OfflineEvdSBND::analyze(art::Event const& e)
{
  std::vector<art::Ptr<raw::RawDigit>> _raw_digits_handle;

  // Implementation of required member function here.

  int run = e.run();
  int subrun = e.subRun();
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
         qmax = qmax_2;
         qmin = qmin_2;
    }
    else if (plane == 0) { 
        qmax = qmax_0;
        qmin = qmin_0;
    }
    else if (plane == 1) { 
        qmax = qmax_1;
        qmin = qmin_1;
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
      //std::cout << "[tpcAnalysis::OfflineEvdSBND::analyze] adc_vec[" << i << "] : " << adc_vec[i] << ", rd->GetPedestal() : " << rd->GetPedestal() << std::endl;
      int bin = hrawadc[plane + 3*tpc]->GetBin(wire+0.1,i+0.1);
      double q = adc_vec[i] - rd->GetPedestal();
      if (plane == 2){
           qmax = qmax_2;
           qmin = qmin_2;
      }
      else if (plane == 0) { 
          qmax = qmax_0;
          qmin = qmin_0;
      }
      else if (plane == 1) { 
          qmax = qmax_1;
          qmin = qmin_1;
      }
      if (q>qmax) q = qmax;
      if (q<qmin) q = qmin;
      hrawadc[plane + 3*tpc]->SetBinContent(bin, q);
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
//    t.DrawLatex(0.01,0.01,Form("%04d/%02d/%02d %02d:%02d:%02d %s",
//			       tlocal->tm_year+1900,
//			       tlocal->tm_mon+1,
//			       tlocal->tm_mday,
//			       tlocal->tm_hour,
//			       tlocal->tm_min,
//			       tlocal->tm_sec,
//			       tlocal->tm_isdst?"CDT":"CST"));
    t.DrawLatex(0.01,0.01,Form("%s",oss.str().c_str()));
    can->Print(Form("/daq/log/TPC_EVD/sbnd_run%d_subrun%d_event%d_tpc%d_plane%d.png",run,subrun, event, tpc,plane));
    //can->Print(Form("./sbnd_tpc%d_plane%d.pdf",tpc,plane));
    delete hrawadc[i];
  }
  delete can;
  SendImages(e);
}

void tpcAnalysis::OfflineEvdSBND::beginJob()
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

DEFINE_ART_MODULE(tpcAnalysis::OfflineEvdSBND)
