#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <sstream>
#include <TVirtualFFT.h>
#include "TStopwatch.h"
#include "TROOT.h"
#include "TTree.h"
#include "TH1F.h"
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

#include "sbndqm/dqmAnalysis/TPC/ChannelData.hh"
#include "sbndqm/Decode/TPC/HeaderData.hh"
#include "sbndqm/dqmAnalysis/TPC/Analysis.hh"

#include "sbndaq-online/helpers/Utilities.h"

/* Uses the Analysis class to print stuff to file
*/
namespace tpcAnalysis {
  class TPCWaveformCreator;
}


class tpcAnalysis::TPCWaveformCreator : public art::EDAnalyzer {
public:
  explicit TPCWaveformCreator(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base.
  // classes without bare pointers or other resource use. Plugins should not be copied or assigned.
   TPCWaveformCreator(TPCWaveformCreator const &) = delete;
   TPCWaveformCreator(TPCWaveformCreator &&) = delete;
   TPCWaveformCreator & operator = (TPCWaveformCreator const &) = delete;
   TPCWaveformCreator & operator = (TPCWaveformCreator &&) = delete;
   virtual void analyze(art::Event const & e) override;

private:
  TTree *_output;
  TH1D *Times;
  TStopwatch timer;
  TStopwatch master;
  double rSum = 0.0;
  double sSum = 0.0;
  double Ped = 0.0;
  double sum = 0.0;
  double stringTime = 0.0;
  double stringSum = 0.0;
  redisContext* context;
  double makeStrings(raw::RawDigit const&);

  std::string fRedisHostname;
  int         fRedisPort;
  
};

double tpcAnalysis::TPCWaveformCreator::makeStrings(raw::RawDigit const& rd){ 
   TStopwatch sendString;
   TStopwatch redisString;
   
   if (rd.Channel() == 0){
     sSum = 0;
     rSum =0;
   }
   sendString.Start();
   // store the waveform and fft's
   // also delete old lists
    redisAppendCommand(context, "DEL snapshot:waveform:wire:%i", rd.Channel());

   // we're gonna put the whole waveform into one very large list 
   // allocate enough space for it 
   // Assume at max 4 chars per int plus a space each plus another 50 chars to store the base of the command
   auto waveform = rd.ADCs();
   size_t buffer_len = waveform.size() * 10 + 50;
   char *buffer = new char[buffer_len];
   
   // print in the base of the command
   size_t print_len = sprintf(buffer, "RPUSH snapshot:waveform:wire:%i", rd.Channel());
   char *buffer_index = buffer + print_len;

   // throw in all of the data points
   for (int16_t dat: waveform) {
     print_len += sprintf(buffer_index, " %i", dat); 
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
     std::cout<<" Time to read the buffer to redis is "<<sSum <<" seconds."<<std::endl;
   }
   redisString.Start();
   redisGetReply(context,NULL);
   redisGetReply(context,NULL);                                                                                                                         
   // delete the buffer                                                                                                                         
   delete buffer;
   redisString.Stop();
   rSum = rSum + redisString.RealTime();
   if (rd.Channel() == 575) {
     std::cout<<" Time to receive the replies from  redis is "<<rSum <<" seconds."<<std::endl;
   }
   
   if (rd.Channel() == 575) {
     std::cout<<" Total time "<<sSum + rSum <<" seconds."<<std::endl;
   }
   double time = rSum + sSum;
   
   return time;
}

tpcAnalysis::TPCWaveformCreator::TPCWaveformCreator(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  fRedisHostname(p.get<std::string>("RedisHostname","icarus-db02.fnal.gov")),
  fRedisPort(p.get<int>("RedisPort",6379))
{  
  context =  sbndaq::Connect2Redis(fRedisHostname,fRedisPort);//to make the configure options w/ password??  later 

  art::ServiceHandle<art::TFileService> tfs; 
  Times = tfs->make<TH1D>("Times","Channel_Times",100,0,2);   

}
  
void tpcAnalysis::TPCWaveformCreator::analyze(art::Event const & evt) {
  master.Start();
  art::EventNumber_t eventNumber = evt.id().event();
  //get the raw digits from the event
  timer.Start();
  art::Handle<std::vector<raw::RawDigit>> raw_digits_handle;
  evt.getByLabel("daq", raw_digits_handle);
  timer.Stop();
  //  std::cout << " Time to read in raw digits " << timer.RealTime()<< " seconds."<<std::endl;
   //wes prefers working with vectors, but you can use the handle/pointer if you want
  timer.Start(); 
  std::vector<raw::RawDigit> const& raw_digits_vector(*raw_digits_handle);
   // std::cout << "Size of vector is " << raw_digits_vector.size() << " or " << raw_digits_handle->size() << std::endl;
  timer.Stop();
  // std::cout << " Time to read in vector " << timer.RealTime()<<" seconds."<< std::endl;

  art::ServiceHandle<art::TFileService> tfs; 
  std::vector<TH1F*> hist_vector_eventNumber;
 
   // std::cout <<  "Run " << evt.run() << ", subrun " << evt.subRun()
   //        << ", event " << eventNumber << " has " << raw_digits_handle->size()
   //	     << " channels"<< std::endl;
   timer.Start();
 
   for(size_t i_d=0; i_d<raw_digits_vector.size(); ++i_d){ 
     // using 'auto' here because I'm too lazy to type raw::RawDigit                                                                                  
     //raw::RawDigit const& rd = raw_digits_vector[i_d];
     auto const& rd = raw_digits_vector[i_d];
     stringTime = makeStrings(rd);
     // std::cout<<"Channel " << rd.Channel() << ": time for the string to be created is " <<stringTime<<" seconds."<<std::endl;
     Times->Fill(stringTime);
    
     //std::cout << "Looking at " << i_d << " rawdigit. Channel number is ... " << rd.Channel() << std::endl;  
     std::stringstream ss_hist_title,ss_hist_name;
     ss_hist_title << "(Run,Event,rawdigit,Channel) "
    		   << evt.run() <<","
		   <<eventNumber << ","
		   <<i_d <<","
		   <<rd.Channel();

     ss_hist_name << "Waveform_"
	          <<eventNumber <<"_"
	          <<i_d;


     //FFT                                                                                                                                         

     /*          int NDIM = 4096;
     TVirtualFFT *fftr2c = TVirtualFFT::FFT(1, &NDIM, "R2C EX K");

     //OK, let's try a Fourier transform now  
     //first, we need a vector of doubles of the same size, to use as input                                       
     std::vector<double> doubleVec(rd.ADCs().begin(),rd.ADCs().end());

     std::cout << "Made double vec ... " << doubleVec.size() << std::endl;
     //now run the FFT ... we already set it up!                                                                                                 
     fftr2c->SetPoints(doubleVec.data());

     fftr2c->Transform();


     //make histograms FFT
     TString h_name,h_title;
     h_name.Form("h_re_%d_%d_%d",(int)(evt.id().event()),(int)((rd.Channel() & 0xff00) >> 8),(int)(rd.Channel() & 0xff));
     h_title.Form("FFT, Real: Event %d, Board %d, Channel %d",
		  (int)(evt.id().event()),(int)((rd.Channel() & 0xff00) >> 8),(int)(rd.Channel() & 0xff));
     TH1 *hfft_re = 0;
     hfft_re = TH1::TransformHisto(fftr2c,hfft_re,"RE");
     hfft_re->SetName(h_name);
     hfft_re->SetTitle(h_title);
     hfft_re->GetXaxis()->SetRange(0,2049);
      
      
     h_name.Form("h_im_%d_%d_%d",(int)(evt.id().event()),(int)((rd.Channel() & 0xff00) >> 8),(int)(rd.Channel() & 0xff));
     h_title.Form("FFT, Imaginary: Event %d, Board %d, Channel %d",
		  (int)(evt.id().event()),(int)((rd.Channel() & 0xff00) >> 8),(int)(rd.Channel() & 0xff));
     TH1 *hfft_im = 0;
     hfft_im = TH1::TransformHisto(fftr2c,hfft_im,"IM");
     hfft_im->SetName(h_name);
     hfft_im->SetTitle(h_title);
     hfft_im->GetXaxis()->SetRange(0,2049);

     h_name.Form("h_mag_%d_%d_%d",(int)(evt.id().event()),(int)((rd.Channel() & 0xff00) >> 8),(int)(rd.Channel() & 0xff));
     h_title.Form("FFT, Magnitude: Event %d, Board %d, Channel %d",
		  (int)(evt.id().event()),(int)((rd.Channel() & 0xff00) >> 8),(int)(rd.Channel() & 0xff));
     TH1 *hfft_m = 0;
     hfft_m = TH1::TransformHisto(fftr2c,hfft_m,"MAG");
     hfft_m->SetName(h_name);
     hfft_m->SetTitle(h_title);
     hfft_m->GetXaxis()->SetRange(0,2049);

     hfft_re->Write();
     hfft_im->Write();
     hfft_m->Write();

     delete hfft_re;
     delete hfft_im;
     delete hfft_m;
     
     */
     //make histograms Waveform      
     // std::cout << "going to create histogram " << ss_hist_name.str()<< std::endl;


     hist_vector_eventNumber.push_back(tfs->make<TH1F>(ss_hist_name.str().c_str(),ss_hist_title.str().c_str(),4096,0,4096));
     //          std::cout << "Created histo. Total histos is now  " << hist_vector_eventNumber.size()<< std::endl;
     for(size_t i_t=0; i_t< rd.Samples(); ++i_t){
       short my_adc_value = rd.ADC(i_t); 
       //       std::cout << " Wave val " << my_adc_value<<std::endl; 
       if(my_adc_value!=0) hist_vector_eventNumber.back()->SetBinContent(i_t+1,my_adc_value);

     }
  }

   timer.Stop();
   std::cout << " Time to create three types of  histograms is " << timer.RealTime()<< " seconds."<< std::endl;

   master.Stop();
   std::cout << " Event " << eventNumber<< " is now completed." << std::endl;
   std::cout << " The total time for this event is " << master.RealTime()<<" seconds."<< std::endl;
   sum = sum +master.RealTime();
   std::cout  << " The overall total time accross all events is " << sum <<" seconds."<< std::endl;	    

  
}


DEFINE_ART_MODULE(tpcAnalysis::TPCWaveformCreator)	 
				  
  //fin

