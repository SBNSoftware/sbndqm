//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Fri Jul 26 08:05:06 2019 by ROOT version 6.16/00
// from TTree event/event
// found on file: out.root
//////////////////////////////////////////////////////////

#ifndef anaUncorrelated_h
#define anaUncorrelated_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1D.h>

// Header file for the classes stored in the TTree if any.
#include "vector"
#include "sbndqm/dqmAnalysis/TPC/ChannelData.hh"

class anaUncorrelated {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

// Fixed size dimensions of array or collections stored in the TTree if any.
   static constexpr Int_t kMaxchannel_data = 576;
   static constexpr Int_t kMaxboard_data = 9;
   static constexpr Int_t kMaxuncorrelated_data = 576;

   // Declaration of leaf types
   Int_t           channel_data_;
   UInt_t          channel_data_channel_no[kMaxchannel_data];   //[channel_data_]
   Bool_t          channel_data_empty[kMaxchannel_data];   //[channel_data_]
   Short_t         channel_data_baseline[kMaxchannel_data];   //[channel_data_]
   Float_t         channel_data_rms[kMaxchannel_data];   //[channel_data_]
   Float_t         channel_data_next_channel_dnoise[kMaxchannel_data];   //[channel_data_]
   Float_t         channel_data_threshold[kMaxchannel_data];   //[channel_data_]
   vector<short>   channel_data_waveform[kMaxchannel_data];
   vector<double>  channel_data_fft_real[kMaxchannel_data];
   vector<double>  channel_data_fft_imag[kMaxchannel_data];
 //vector<tpcAnalysis::PeakFinder::Peak> channel_data_peaks[kMaxchannel_data];
 //vector<array<unsigned int,2> > channel_data_noise_ranges[kMaxchannel_data];
   Float_t         channel_data_mean_peak_height[kMaxchannel_data];   //[channel_data_]
   Float_t         channel_data_occupancy[kMaxchannel_data];   //[channel_data_]
   Double_t        channel_data_fourier_real[kMaxchannel_data][2048];   //[channel_data_]
   Double_t        channel_data_fourier_imag[kMaxchannel_data][2048];   //[channel_data_]
   Int_t           board_data_;
   UInt_t          board_data_channel_no[kMaxboard_data];   //[board_data_]
   Bool_t          board_data_empty[kMaxboard_data];   //[board_data_]
   Short_t         board_data_baseline[kMaxboard_data];   //[board_data_]
   Float_t         board_data_rms[kMaxboard_data];   //[board_data_]
   Float_t         board_data_next_channel_dnoise[kMaxboard_data];   //[board_data_]
   Float_t         board_data_threshold[kMaxboard_data];   //[board_data_]
   vector<short>   board_data_waveform[kMaxboard_data];
   vector<double>  board_data_fft_real[kMaxboard_data];
   vector<double>  board_data_fft_imag[kMaxboard_data];
 //vector<tpcAnalysis::PeakFinder::Peak> board_data_peaks[kMaxboard_data];
 //vector<array<unsigned int,2> > board_data_noise_ranges[kMaxboard_data];
   Float_t         board_data_mean_peak_height[kMaxboard_data];   //[board_data_]
   Float_t         board_data_occupancy[kMaxboard_data];   //[board_data_]
   Double_t        board_data_fourier_real[kMaxboard_data][2048];   //[board_data_]
   Double_t        board_data_fourier_imag[kMaxboard_data][2048];   //[board_data_]
   Int_t           uncorrelated_data_;
   UInt_t          uncorrelated_data_channel_no[kMaxuncorrelated_data];   //[uncorrelated_data_]
   Bool_t          uncorrelated_data_empty[kMaxuncorrelated_data];   //[uncorrelated_data_]
   Short_t         uncorrelated_data_baseline[kMaxuncorrelated_data];   //[uncorrelated_data_]
   Float_t         uncorrelated_data_rms[kMaxuncorrelated_data];   //[uncorrelated_data_]
   Float_t         uncorrelated_data_next_channel_dnoise[kMaxuncorrelated_data];   //[uncorrelated_data_]
   Float_t         uncorrelated_data_threshold[kMaxuncorrelated_data];   //[uncorrelated_data_]
   vector<short>   uncorrelated_data_waveform[kMaxuncorrelated_data];
   vector<double>  uncorrelated_data_fft_real[kMaxuncorrelated_data];
   vector<double>  uncorrelated_data_fft_imag[kMaxuncorrelated_data];
 //vector<tpcAnalysis::PeakFinder::Peak> uncorrelated_data_peaks[kMaxuncorrelated_data];
 //vector<array<unsigned int,2> > uncorrelated_data_noise_ranges[kMaxuncorrelated_data];
   Float_t         uncorrelated_data_mean_peak_height[kMaxuncorrelated_data];   //[uncorrelated_data_]
   Float_t         uncorrelated_data_occupancy[kMaxuncorrelated_data];   //[uncorrelated_data_]
   Double_t        uncorrelated_data_fourier_real[kMaxuncorrelated_data][2048];   //[uncorrelated_data_]
   Double_t        uncorrelated_data_fourier_imag[kMaxuncorrelated_data][2048];   //[uncorrelated_data_]

   // List of branches
   TBranch        *b_channel_data_;   //!
   TBranch        *b_channel_data_channel_no;   //!
   TBranch        *b_channel_data_empty;   //!
   TBranch        *b_channel_data_baseline;   //!
   TBranch        *b_channel_data_rms;   //!
   TBranch        *b_channel_data_next_channel_dnoise;   //!
   TBranch        *b_channel_data_threshold;   //!
   TBranch        *b_channel_data_waveform;   //!
   TBranch        *b_channel_data_fft_real;   //!
   TBranch        *b_channel_data_fft_imag;   //!
   TBranch        *b_channel_data_mean_peak_height;   //!
   TBranch        *b_channel_data_occupancy;   //!
   TBranch        *b_channel_data_fourier_real;   //!
   TBranch        *b_channel_data_fourier_imag;   //!
   TBranch        *b_board_data_;   //!
   TBranch        *b_board_data_channel_no;   //!
   TBranch        *b_board_data_empty;   //!
   TBranch        *b_board_data_baseline;   //!
   TBranch        *b_board_data_rms;   //!
   TBranch        *b_board_data_next_channel_dnoise;   //!
   TBranch        *b_board_data_threshold;   //!
   TBranch        *b_board_data_waveform;   //!
   TBranch        *b_board_data_fft_real;   //!
   TBranch        *b_board_data_fft_imag;   //!
   TBranch        *b_board_data_mean_peak_height;   //!
   TBranch        *b_board_data_occupancy;   //!
   TBranch        *b_board_data_fourier_real;   //!
   TBranch        *b_board_data_fourier_imag;   //!
   TBranch        *b_uncorrelated_data_;   //!
   TBranch        *b_uncorrelated_data_channel_no;   //!
   TBranch        *b_uncorrelated_data_empty;   //!
   TBranch        *b_uncorrelated_data_baseline;   //!
   TBranch        *b_uncorrelated_data_rms;   //!
   TBranch        *b_uncorrelated_data_next_channel_dnoise;   //!
   TBranch        *b_uncorrelated_data_threshold;   //!
   TBranch        *b_uncorrelated_data_waveform;   //!
   TBranch        *b_uncorrelated_data_fft_real;   //!
   TBranch        *b_uncorrelated_data_fft_imag;   //!
   TBranch        *b_uncorrelated_data_mean_peak_height;   //!
   TBranch        *b_uncorrelated_data_occupancy;   //!
   TBranch        *b_uncorrelated_data_fourier_real;   //!
   TBranch        *b_uncorrelated_data_fourier_imag;   //!

   anaUncorrelated(TTree *tree=0);
   virtual ~anaUncorrelated();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);

TH1D* ffthisto;
   TH1D* fftrhisto;
TH1D* fft2histo;
   TH1D* fft2Bhisto;
TH1D* fft2Uhisto;
 


  TH1D* fftBhisto;
   TH1D* fftBrhisto; 
  TH1D* fftBihisto; 
TH1D* fftihisto;

  TH1D* fftUhisto;
   TH1D* fftUrhisto;
TH1D* fftUihisto;
TH1D* RMShisto;
TH1D* RMSBhisto;
TH1D* RMSUhisto;
};

#endif

#ifdef anaUncorrelated_cxx
anaUncorrelated::anaUncorrelated(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("out.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("out.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("out.root:/OfflineAnalysis");
      dir->GetObject("event",tree);

   }
   Init(tree);
}

anaUncorrelated::~anaUncorrelated()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t anaUncorrelated::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t anaUncorrelated::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void anaUncorrelated::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("channel_data", &channel_data_, &b_channel_data_);
   fChain->SetBranchAddress("channel_data.channel_no", channel_data_channel_no, &b_channel_data_channel_no);
   fChain->SetBranchAddress("channel_data.empty", channel_data_empty, &b_channel_data_empty);
   fChain->SetBranchAddress("channel_data.baseline", channel_data_baseline, &b_channel_data_baseline);
   fChain->SetBranchAddress("channel_data.rms", channel_data_rms, &b_channel_data_rms);
   fChain->SetBranchAddress("channel_data.next_channel_dnoise", channel_data_next_channel_dnoise, &b_channel_data_next_channel_dnoise);
   fChain->SetBranchAddress("channel_data.threshold", channel_data_threshold, &b_channel_data_threshold);
   fChain->SetBranchAddress("channel_data.waveform", channel_data_waveform, &b_channel_data_waveform);
   fChain->SetBranchAddress("channel_data.fft_real", channel_data_fft_real, &b_channel_data_fft_real);
   fChain->SetBranchAddress("channel_data.fft_imag", channel_data_fft_imag, &b_channel_data_fft_imag);
   fChain->SetBranchAddress("channel_data.mean_peak_height", channel_data_mean_peak_height, &b_channel_data_mean_peak_height);
   fChain->SetBranchAddress("channel_data.occupancy", channel_data_occupancy, &b_channel_data_occupancy);
   fChain->SetBranchAddress("channel_data.fourier_real[2048]", channel_data_fourier_real, &b_channel_data_fourier_real);
   fChain->SetBranchAddress("channel_data.fourier_imag[2048]", channel_data_fourier_imag, &b_channel_data_fourier_imag);
   fChain->SetBranchAddress("board_data", &board_data_, &b_board_data_);
   fChain->SetBranchAddress("board_data.channel_no", board_data_channel_no, &b_board_data_channel_no);
   fChain->SetBranchAddress("board_data.empty", board_data_empty, &b_board_data_empty);
   fChain->SetBranchAddress("board_data.baseline", board_data_baseline, &b_board_data_baseline);
   fChain->SetBranchAddress("board_data.rms", board_data_rms, &b_board_data_rms);
   fChain->SetBranchAddress("board_data.next_channel_dnoise", board_data_next_channel_dnoise, &b_board_data_next_channel_dnoise);
   fChain->SetBranchAddress("board_data.threshold", board_data_threshold, &b_board_data_threshold);
   fChain->SetBranchAddress("board_data.waveform", board_data_waveform, &b_board_data_waveform);
   fChain->SetBranchAddress("board_data.fft_real", board_data_fft_real, &b_board_data_fft_real);
   fChain->SetBranchAddress("board_data.fft_imag", board_data_fft_imag, &b_board_data_fft_imag);
   fChain->SetBranchAddress("board_data.mean_peak_height", board_data_mean_peak_height, &b_board_data_mean_peak_height);
   fChain->SetBranchAddress("board_data.occupancy", board_data_occupancy, &b_board_data_occupancy);
   fChain->SetBranchAddress("board_data.fourier_real[2048]", board_data_fourier_real, &b_board_data_fourier_real);
   fChain->SetBranchAddress("board_data.fourier_imag[2048]", board_data_fourier_imag, &b_board_data_fourier_imag);
   fChain->SetBranchAddress("uncorrelated_data", &uncorrelated_data_, &b_uncorrelated_data_);
   fChain->SetBranchAddress("uncorrelated_data.channel_no", uncorrelated_data_channel_no, &b_uncorrelated_data_channel_no);
   fChain->SetBranchAddress("uncorrelated_data.empty", uncorrelated_data_empty, &b_uncorrelated_data_empty);
   fChain->SetBranchAddress("uncorrelated_data.baseline", uncorrelated_data_baseline, &b_uncorrelated_data_baseline);
   fChain->SetBranchAddress("uncorrelated_data.rms", uncorrelated_data_rms, &b_uncorrelated_data_rms);
   fChain->SetBranchAddress("uncorrelated_data.next_channel_dnoise", uncorrelated_data_next_channel_dnoise, &b_uncorrelated_data_next_channel_dnoise);
   fChain->SetBranchAddress("uncorrelated_data.threshold", uncorrelated_data_threshold, &b_uncorrelated_data_threshold);
   fChain->SetBranchAddress("uncorrelated_data.waveform", uncorrelated_data_waveform, &b_uncorrelated_data_waveform);
   fChain->SetBranchAddress("uncorrelated_data.fft_real", uncorrelated_data_fft_real, &b_uncorrelated_data_fft_real);
   fChain->SetBranchAddress("uncorrelated_data.fft_imag", uncorrelated_data_fft_imag, &b_uncorrelated_data_fft_imag);
   fChain->SetBranchAddress("uncorrelated_data.mean_peak_height", uncorrelated_data_mean_peak_height, &b_uncorrelated_data_mean_peak_height);
   fChain->SetBranchAddress("uncorrelated_data.occupancy", uncorrelated_data_occupancy, &b_uncorrelated_data_occupancy);
   fChain->SetBranchAddress("uncorrelated_data.fourier_real[2048]", uncorrelated_data_fourier_real, &b_uncorrelated_data_fourier_real);
   fChain->SetBranchAddress("uncorrelated_data.fourier_imag[2048]", uncorrelated_data_fourier_imag, &b_uncorrelated_data_fourier_imag);
   Notify();
}

Bool_t anaUncorrelated::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void anaUncorrelated::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t anaUncorrelated::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef anaUncorrelated_cxx
