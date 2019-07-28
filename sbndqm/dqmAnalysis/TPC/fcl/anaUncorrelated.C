#define anaUncorrelated_cxx
#include "anaUncorrelated.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>

void anaUncorrelated::Loop()
{
//   In a ROOT session, you can do:
//      root> .L anaUncorrelated.C
//      root> anaUncorrelated t
//      root> t.GetEntry(12); // Fill t data members with entry number 12
//      root> t.Show();       // Show values of entry 12
//      root> t.Show(16);     // Read and show values of entry 16
//      root> t.Loop();       // Loop on all entries
//

//     This is the loop skeleton where:
//    jentry is the global entry number in the chain
//    ientry is the entry number in the current Tree
//  Note that the argument to GetEntry must be:
//    jentry for TChain::GetEntry
//    ientry for TTree::GetEntry and TBranch::GetEntry
//
//       To read only selected branches, Insert statements like:
// METHOD1:
//    fChain->SetBranchStatus("*",0);  // disable all branches
//    fChain->SetBranchStatus("branchname",1);  // activate branchname
// METHOD2: replace line
//    fChain->GetEntry(jentry);       //read all branches
//by  b_branchname->GetEntry(ientry); //read only this branch
   if (fChain == 0) return;
   
ffthisto=new TH1D("ffthisto","ffthisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
fft2histo=new TH1D("fft2histo","fft2histo",2048,0.5*1250./2048.,2048.5*1250./2048.);
   fftrhisto=new TH1D("fftrhisto","fftrhisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
  fftihisto=new TH1D("fftihisto","fftihisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
fftBhisto=new TH1D("fftBhisto","fftBhisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
fft2Bhisto=new TH1D("fft2Bhisto","fft2Bhisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
   fftBrhisto=new TH1D("fftBrhisto","fftBrhisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
 fftBihisto=new TH1D("fftBihisto","fftBihisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
fftUhisto=new TH1D("fftUhisto","fftUhisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
fft2Uhisto=new TH1D("fft2Uhisto","fft2Uhisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
   fftUrhisto=new TH1D("fftUrhisto","fftUrhisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
 fftUihisto=new TH1D("fftUihisto","fftUihisto",2048,0.5*1250./2048.,2048.5*1250./2048.);
RMShisto=new TH1D("RMShisto","RMShisto",100,0.,10.);
RMSBhisto=new TH1D("RMSBhisto","RMSBhisto",100,0.,10.);
RMSUhisto=new TH1D("RMSUhisto","RMSUhisto",100,0.,10.);
   Long64_t nentries = fChain->GetEntriesFast();

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
    //if(ientry>0)break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;


 for(int jc=0;jc<kMaxchannel_data;jc++) {
  // cout << " maxchannel " << kMaxchannel_data << endl;
int nch=channel_data_channel_no[jc];
//cout << " nch " << nch << endl;
   // float fftr[]=(channel_data_fourier_real)[jc];
     //float ffti[]=(channel_data_fourier_imag)[jc];
//vector<short> wave=(channel_data_waveform)[jc];
    // std::cout << " fftr size " << fftr.size() << std::endl;
//cout << " wave size " << wave.size() << endl;
      for(unsigned int jf=0;jf<2048;jf++) { 
//std::cout << " jf " << jf << std::endl;
     double fr=(channel_data_fourier_real[jc])[jf];
     double fi=(channel_data_fourier_imag[jc])[jf];
     double fft=sqrt(fr*fr+fi*fi);
     double fft2=(fr*fr+fi*fi);
    //std::cout << " jf " << jf << " fft " << fft << std::endl;
     ffthisto->Fill(jf*1250./2048.,fft);
fft2histo->Fill(jf*1250./2048.,fft2);
     fftrhisto->Fill(jf*1250./2048.,fr);
 fftihisto->Fill(jf*1250./2048.,fi);
      }
double rms=(channel_data_rms[jc]);
//std::cout <<  "rms " << rms << std::endl;
RMShisto->Fill(rms);
//std::cout << " after filling " << std::endl;
}

 for(int jc=0;jc<kMaxchannel_data/64;jc++) {
 //  cout << " board jc " << jc << endl;

      for(unsigned int jf=0;jf<2048;jf++) { 

     double fr=(board_data_fourier_real[jc])[jf];
     double fi=(board_data_fourier_imag[jc])[jf];
     double fft=sqrt(fr*fr+fi*fi);
     double fft2=(fr*fr+fi*fi);
    //cout << " jf " << jf << " fft " << fft << endl;
     fftBhisto->Fill(jf*1250./2048.,fft);
fft2Bhisto->Fill(jf*1250./2048.,fft2);
     fftBrhisto->Fill(jf*1250./2048.,fr);
fftBihisto->Fill(jf*1250./2048.,fi);
      }
double rms=(board_data_rms[jc]);
RMSBhisto->Fill(rms);
}
for(int jc=0;jc<kMaxchannel_data;jc++) {
  // cout << " maxchannel " << kMaxchannel_data << endl;
      for(unsigned int jf=0;jf<2048;jf++) { 
//std::cout << " jf " << jf << std::endl;
     double fr=(uncorrelated_data_fourier_real[jc])[jf];
     double fi=(uncorrelated_data_fourier_imag[jc])[jf];
     double fft=sqrt(fr*fr+fi*fi);
     double fft2=(fr*fr+fi*fi);
    //std::cout << " jf " << jf << " fft " << fft << std::endl;
     fftUhisto->Fill(jf*1250./2048.,fft);
 fft2Uhisto->Fill(jf*1250./2048.,fft2);
     fftUrhisto->Fill(jf*1250./2048.,fr);
     fftUihisto->Fill(jf*1250./2048.,fi);
      }
double rms=(uncorrelated_data_rms[jc]);
RMSUhisto->Fill(rms);
}

      // if (Cut(ientry) < 0) continue;
   }
ffthisto->Scale(1./(nentries*kMaxchannel_data));
fftrhisto->Scale(1./(nentries*kMaxchannel_data));
fftUhisto->Scale(1./(nentries*kMaxchannel_data));
fft2histo->Scale(1./(nentries*kMaxchannel_data));
fft2Bhisto->Scale(1./(nentries*kMaxchannel_data/64));
fft2Uhisto->Scale(1./(nentries*kMaxchannel_data));
fftUrhisto->Scale(1./(nentries*kMaxchannel_data));
fftUihisto->Scale(1./(nentries*kMaxchannel_data));
fftihisto->Scale(1./(nentries*kMaxchannel_data));
fftBhisto->Scale(1./(nentries*kMaxchannel_data/64));
fftBrhisto->Scale(1./(nentries*kMaxchannel_data/64));
fftBihisto->Scale(1./(nentries*kMaxchannel_data/64));
double avRMS=RMShisto->GetMean();
double avRMSB=RMSBhisto->GetMean();
double avRMSU=RMSUhisto->GetMean();
TFile* file=new TFile("FFtsbn.root","RECREATE");

ffthisto->Write();
fftrhisto->Write();
fftihisto->Write();
fft2histo->Write();
fft2Uhisto->Write();
fft2Bhisto->Write();
fftBhisto->Write();
fftBrhisto->Write();
fftUhisto->Write();
fftUrhisto->Write();
fftUihisto->Write();
fftBihisto->Write();
RMShisto->Write();
RMSUhisto->Write();
RMSBhisto->Write();
file->Close();
}
