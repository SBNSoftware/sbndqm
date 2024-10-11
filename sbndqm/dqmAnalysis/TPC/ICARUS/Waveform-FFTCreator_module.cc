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

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Analysis.hh"

#include "sbndaq-redis-plugin/Utilities.h"

/* Uses the Analysis class to print stuff to file
*/
namespace tpcAnalysis {
  class TPCWaveformAndFftRedis;
}

class tpcAnalysis::TPCWaveformAndFftRedis : public art::EDAnalyzer {
public:
  explicit TPCWaveformAndFftRedis(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base.
  // classes without bare pointers or other resource use. Plugins should not be copied or assigned.
   TPCWaveformAndFftRedis(TPCWaveformAndFftRedis const &) = delete;
   TPCWaveformAndFftRedis(TPCWaveformAndFftRedis &&) = delete;
   TPCWaveformAndFftRedis & operator = (TPCWaveformAndFftRedis const &) = delete;
   TPCWaveformAndFftRedis & operator = (TPCWaveformAndFftRedis &&) = delete;
   virtual void analyze(art::Event const & e) override;

private:
  tpcAnalysis::Analysis _analysis;
  TTree *_output;
  TStopwatch timer;
  TStopwatch master;
  TStopwatch first;
  int a = 0;
  int Ped;
  char option[20];
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
  redisContext* context;
  double makeStrings(raw::RawDigit const&,int);
  double makeFFT(raw::RawDigit const&,int);

  std::string fRedisHostname;
  int         fRedisPort;

 };
double tpcAnalysis::TPCWaveformAndFftRedis::makeFFT(raw::RawDigit const& rd,int Ped){
   TStopwatch sendFFT;
   TStopwatch redisFFT;
   int NDIM = 4096;
   TVirtualFFT *fftr2c = TVirtualFFT::FFT(1, &NDIM, "R2C EX K");
   

  if (rd.Channel() == 0){
    fSum = 0;
    FSum = 0;
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
  delete buffer;
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

double tpcAnalysis::TPCWaveformAndFftRedis::makeStrings(raw::RawDigit const& rd,int Ped){ 
   TStopwatch sendString;
   TStopwatch redisString;
   // std::cout<<"Now Ped"<<Ped<<std::endl;   
   if (rd.Channel() == 0){
     sSum = 0;
     rSum = 0;
   }
   sendString.Start();
   // store the waveform and also delete old lists
   redisAppendCommand(context, "DEL snapshot:waveform:wire:%i", rd.Channel());
   // we're gonna put the whole waveform into one very large list 
   // allocate enough space for it 
   // Assume at max 4 chars per int plus a space each plus another 50 chars to store the base of the command
   auto waveform =  rd.ADCs();
   size_t buffer_len = waveform.size() * 10 + 50;
   char *buffer = new char[buffer_len];
      // print in the base of the command
   size_t print_len = sprintf(buffer, "RPUSH snapshot:waveform:wire:%i", rd.Channel());
   char *buffer_index = buffer + print_len;
   // throw in all of the data points
   for (int16_t dat: waveform) {
     print_len += sprintf(buffer_index, " %i", dat - Ped); 
     buffer_index = buffer + print_len;
     if (print_len >= buffer_len - 1) {
       std::cerr << "ERROR: BUFFER OVERFLOW IN WAVEFORM DATA" << std::endl;
       std::exit(1);
     }
   }
   // null terminate the string
   *buffer_index = '\0';
   redisAppendCommand(context, buffer);
   sendString.Stop();
   sSum = sSum + sendString.RealTime();
   if (rd.Channel() == 575) {
     //  std::cout<<" Time to create the buffer for  redis is "<<sSum <<" seconds."<<std::endl;
   }
   redisString.Start();
   redisGetReply(context,NULL);
   redisGetReply(context,NULL);                                                                                                
   // delete the buffer                                                                                                                         
   delete buffer;
   redisString.Stop();
   rSum = rSum + redisString.RealTime();
   if (rd.Channel() == 575) {
     //  std::cout<<" Time to send the strings to redis is "<<rSum <<" seconds."<<std::endl;
   }
   if (rd.Channel() == 575) {
     std::cout<<" Total time "<<sSum + rSum <<" seconds."<<std::endl;
   }
   double time = rSum + sSum;
 
   return time;
}

tpcAnalysis::TPCWaveformAndFftRedis::TPCWaveformAndFftRedis(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p), 
  _analysis(p),

  fRedisHostname(p.get<std::string>("RedisHostname","icarus-db02")),
  fRedisPort(p.get<int>("RedisPort",6379))
{
  first.Start();
  context =  sbndaq::Connect2Redis(fRedisHostname,fRedisPort);//to make the configure options w/ password??  later 
  first.Stop();
}
  
void tpcAnalysis::TPCWaveformAndFftRedis::analyze(art::Event const & evt) {
    master.Start();  
   
    char waveform[] = "waveform";
    char fft[] = "fft";
    char both[] = "both";
    
  
    while (a < 1){
      std::cout << "Do you want to read in the waveform, fft, or both? Please respond with waveform, fft, or both. ";
      std::cin.getline (option,20);
      for ( int t = 0; t < (signed)strlen(option); t++){
	option[t] = tolower(option[t]);
      }
	strcpy (option, option);
      if( (strcmp (waveform,option) == 0) || (strcmp (fft,option) == 0) || (strcmp (both,option) == 0)){
	a++;
      }
    }
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
	Ped = Sum/(counter);
	//std::cout<<Ped<<std::endl;
      
      }
	
      if (strcmp (waveform,option) == 0){    
      //calling the Redis string function
      stringTime = makeStrings(rd,Ped);
      
      }
      else if (strcmp (fft,option) == 0){
      //FFT to call the FFT redis function
      FFTtime = makeFFT(rd,Ped);
     
      }
      
      else  if (strcmp (both,option) == 0){
	//calling the Redis string function
	stringTime = makeStrings(rd,Ped);
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

DEFINE_ART_MODULE(tpcAnalysis::TPCWaveformAndFftRedis)	 
				  
  //fin
 
