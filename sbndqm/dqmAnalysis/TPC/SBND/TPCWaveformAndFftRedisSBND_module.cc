#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <TVirtualFFT.h>
#include "TStopwatch.h"
#include "TROOT.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH1D.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Utilities/ExceptionMessages.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art_root_io/TFileService.h"

#include "sbndqm/dqmAnalysis/TPC/ChannelDataSBND.hh"
#include "sbndqm/Decode/TPC/HeaderData.hh"
#include "sbndqm/dqmAnalysis/TPC/AnalysisSBND.hh"

#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"

//#include "sbndqm/DatabaseStorage/Connect.h"
/* Uses the Analysis class to print stuff to file
*/
namespace tpcAnalysis {
  class TPCWaveformAndFftRedisSBND;
}

class tpcAnalysis::TPCWaveformAndFftRedisSBND : public art::EDAnalyzer {
public:
  explicit TPCWaveformAndFftRedisSBND(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base.
  // classes without bare pointers or other resource use. Plugins should not be copied or assigned.
   TPCWaveformAndFftRedisSBND(TPCWaveformAndFftRedisSBND const &) = delete;
   TPCWaveformAndFftRedisSBND(TPCWaveformAndFftRedisSBND &&) = delete;
   TPCWaveformAndFftRedisSBND & operator = (TPCWaveformAndFftRedisSBND const &) = delete;
   TPCWaveformAndFftRedisSBND & operator = (TPCWaveformAndFftRedisSBND &&) = delete;
   virtual void analyze(art::Event const & e) override;

private:
  TTree *_output;
  TStopwatch timer;
  TStopwatch master;
  TStopwatch first;
  int a = 0;
  int Ped;
  //char option[20];
  double setup = 0.0;
  double rSum = 0.0;
  double sSum = 0.0;
  double fSum = 0.0;
  double FSum = 0.0;
  float  Sum;
  float counter = 1;
  float sum = 0.0;
  double stringTime = 0.0;
  double FFTtime = 0.0;
  double stringSum = 0.0;
  void SendWaveform(raw::RawDigit const&);
  redisContext* context;
  double makeFFT(raw::RawDigit const&,int);
 
  std::string fOption;
  bool        fPedestal;
  std::string fWaveformKey;
  double fTickPeriod;
 };

double tpcAnalysis::TPCWaveformAndFftRedisSBND::makeFFT(raw::RawDigit const& rd,int Ped){
   TStopwatch sendFFT;
   TStopwatch redisFFT;
   int NDIM = 4096;
   TVirtualFFT *fftr2c = TVirtualFFT::FFT(1, &NDIM, "R2C EX K");
   

  if (rd.Channel() == 0){
    fSum = 0;
Sum = 0;
  }
   sendFFT.Start();
  // store the waveform and also delete old lists                                                                                           
  redisAppendCommand(context, "DEL snapshot:fft:wire:%i", rd.Channel());
  auto val = rd.ADCs();
  for(size_t i_t=0; i_t< rd.Samples(); ++i_t){
    val[i_t] = rd.ADC(i_t) - Ped;
  }
  std::vector<double> doubleVec(val.begin(),val.end());
  //now run the FFT ... we already set it up!                                                                                               
  fftr2c->SetPoints(doubleVec.data());
  fftr2c->Transform();
  TH1 *hfft_m = 0;
  hfft_m = TH1::TransformHisto(fftr2c,hfft_m,"MAG");
  size_t buffer_len = val.size() * 40 + 50;
  char *buffer = new char[buffer_len];  
  size_t print_len = sprintf(buffer, "RPUSH snapshot:fft:wire:%i", rd.Channel());
  char *buffer_index = buffer + print_len;
  // throw in all of the data points                                                                                                            
  for(int  i_fft=1; i_fft<=hfft_m->GetNbinsX()*(.5); ++i_fft){
    float my_val = hfft_m->GetBinContent(i_fft);  
    print_len += sprintf(buffer_index, " %f",my_val);
    buffer_index = buffer + print_len;
        if (print_len >= buffer_len - 1) {
    std::cerr << "ERROR: BUFFER OVERFLOW IN FFT DATA" << std::endl;
    std::exit(1);
     }
  }
  // null terminate the string                            
  *buffer_index = '\0';
  redisAppendCommand(context, buffer);
  sendFFT.Stop();
   FSum = FSum + sendFFT.RealTime();
   if (rd.Channel() == 575) {
     //   std::cout<<" Time to create the FFT for redis is "<<FSum <<" seconds."<<std::endl;
    }
  redisFFT.Start();
  redisGetReply(context,NULL);
  redisGetReply(context,NULL);
  // delete the buffer                                                                                                                        
  delete[] buffer;
  redisFFT.Stop();
   fSum = fSum + redisFFT.RealTime();
   if (rd.Channel() == 575) {
     // std::cout<<" Time to send the  FFT to redis is "<<fSum <<" seconds."<<std::endl;
   }
   if (rd.Channel() == 575) {
  std::cout<<" Total time "<<fSum + FSum <<" seconds."<<std::endl;
   }
   double Time = fSum + FSum;
   
   return Time;
}

void tpcAnalysis::TPCWaveformAndFftRedisSBND::SendWaveform(raw::RawDigit const& rd) {
  std::string key = "snapshot:" + fWaveformKey + ":wire:" + std::to_string(rd.Channel());
  
  sbndaq::SendWaveform(key, rd.ADCs());
}


tpcAnalysis::TPCWaveformAndFftRedisSBND::TPCWaveformAndFftRedisSBND(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p), 
  fOption(p.get<std::string>("Option","both")),
  fPedestal(p.get<bool>("Pedestal",true)),
  fWaveformKey(p.get<std::string>("WaveformKey", "waveform")),
  fTickPeriod(p.get<double>("TickPeriod", 0.5))

{
}
  
void tpcAnalysis::TPCWaveformAndFftRedisSBND::analyze(art::Event const & evt) {
    master.Start();  
   

  setup = setup + first.RealTime(); 
  art::EventNumber_t eventNumber = evt.id().event();
  //get the raw digits from the event
  timer.Start();
  art::Handle<std::vector<raw::RawDigit>> raw_digits_handle;
  evt.getByLabel("daq", raw_digits_handle);
  timer.Stop();
  //wes prefers working with vectors, but you can use the handle/pointer if you want
  timer.Start(); 
  std::vector<raw::RawDigit> const& raw_digits_vector(*raw_digits_handle);
  // std::cout << "Size of vector is " << raw_digits_vector.size() << " or " << raw_digits_handle->size() << std::endl;
  timer.Stop();
  // std::cout << " Time to read in vector " << timer.RealTime()<<" seconds."<< std::endl;
  art::ServiceHandle<art::TFileService> tfs; 
  std::vector<TH1F*> hist_vector_eventNumber;
   timer.Start();
   for(size_t i_d=0; i_d<raw_digits_vector.size(); ++i_d){ 
     // using 'auto' here because I'm too lazy to type raw::RawDigit                                                                            
     //raw::RawDigit const& rd = raw_digits_vector[i_d];
     auto const& rd = raw_digits_vector[i_d];     
     
      //loop to find sum of ADC values
      for(size_t i_t=0; i_t< rd.Samples(); ++i_t){
	if(i_t == 0) {
	  counter = 1;
	  Sum = 0;
	  Ped = 0;
	}
	short a[100];
	a[i_t] = rd.ADC(i_t);
	Sum+=a[i_t];
	counter++;
       
      }
      //loop for pedestal subtraction and storing that as the waveforms                                                                         
      for(size_t i_t=0; i_t< rd.Samples(); ++i_t){
	if (fPedestal){
	Ped = Sum/(counter);
	//std::cout<<Ped<<std::endl;
	}      
      }
      if (fOption == "waveform"){
	//if (strcmp (waveform,fOption) == 0){    
      //calling the Redis string function
	//      stringTime = makeStrings(rd,Ped);
         SendWaveform(rd);
      }
      else if (fOption == "FFT"){ 
	//if (strcmp (fft,fOption) == 0){
      //FFT to call the FFT redis function
      FFTtime = makeFFT(rd,Ped);
     
      }
      
      else  if (fOption == "both"){
	//(strcmp (both,fOption) == 0){
	//calling the Redis string function
         SendWaveform(rd);
	//FFT to call the FFT redis function 
	FFTtime = makeFFT(rd,Ped);
       
      }
   }      
  
   timer.Stop();
   master.Stop();
   std::cout << " Event " << eventNumber<< " is now completed." << std::endl;
   std::cout << " The total time for this event is " << master.RealTime()<<" seconds."<< std::endl;
   sum = sum +master.RealTime();
   std::cout  << " The overall total time accross all events is " << sum + setup<<" seconds."<< std::endl;	    
  
}

DEFINE_ART_MODULE(tpcAnalysis::TPCWaveformAndFftRedisSBND)	 
				  
  //fin
 
