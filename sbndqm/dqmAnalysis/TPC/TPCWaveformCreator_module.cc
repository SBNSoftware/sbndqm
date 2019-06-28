#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <sstream>
#include "TStopwatch.h"
#include "TROOT.h"
#include "TTree.h"
#include "TH1F.h"

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

/* Uses the Analysis class to print stuff to file
*/
namespace tpcAnalysis {
  class TPCWavefromCreator;
}


class tpcAnalysis::TPCWavefromCreator : public art::EDAnalyzer {
public:
  explicit TPCWavefromCreator(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base.
  // classes without bare pointers or other resource use. Plugins should not be copied or assigned.
   TPCWavefromCreator(TPCWavefromCreator const &) = delete;
   TPCWavefromCreator(TPCWavefromCreator &&) = delete;
   TPCWavefromCreator & operator = (TPCWavefromCreator const &) = delete;
   TPCWavefromCreator & operator = (TPCWavefromCreator &&) = delete;
   virtual void analyze(art::Event const & e) override;

private:
   tpcAnalysis::Analysis _analysis;
   TTree *_output;
   TStopwatch timer;
   TStopwatch master;
};

void makeStrings(raw::RawDigit const& rd){ 
   TStopwatch stringclock;
   stringclock.Start();

   // store the waveform and fft's
   // also delete old lists
   //redisAppendCommand(context, "DEL snapshot:waveform:wire:%i", rd.Channel());

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
   //*buffer_index = '\0';
   //redisAppendCommand(context, buffer);
   //n_commands += 2;
                                                                                                                           
   // delete the buffer                                                                                                                         
   delete buffer;

  stringclock.Stop();
  std::cout << "channel " << rd.Channel() << ": time for the string to be created is " << stringclock.RealTime()<< std::endl;
  
}

tpcAnalysis::TPCWavefromCreator::TPCWavefromCreator(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p){
  master.Start();
}
  
void tpcAnalysis::TPCWavefromCreator::analyze(art::Event const & evt) {
  art::EventNumber_t eventNumber = evt.id().event();
  //get the raw digits from the event
  timer.Start();
  art::Handle<std::vector<raw::RawDigit>> raw_digits_handle;
  evt.getByLabel("daq", raw_digits_handle);
  timer.Stop();
  std::cout << " time to read in raw digits " << timer.RealTime()<< std::endl;
   //wes prefers working with vectors, but you can use the handle/pointer if you want
  timer.Start(); 
  std::vector<raw::RawDigit> const& raw_digits_vector(*raw_digits_handle);
   // std::cout << "Size of vector is " << raw_digits_vector.size() << " or " << raw_digits_handle->size() << std::endl;
  timer.Stop();
  std::cout << " time to read in vector " << timer.RealTime()<< std::endl;

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
     makeStrings(rd);
   
     //     std::cout << "Looking at " << i_d << " rawdigit. Channel number is ... " << rd.Channel() << std::endl;  
     std::stringstream ss_hist_title,ss_hist_name;
     ss_hist_title << "(Run,Event,rawdigit,Channel) "
    		   << evt.run() <<","
		   <<eventNumber << ","
		   <<i_d <<","
		   <<rd.Channel();

     ss_hist_name << "Waveform "
	          <<eventNumber <<"_"
	          <<i_d;

     // std::cout << "going to create histogram " << ss_hist_name.str()<< std::endl;


     hist_vector_eventNumber.push_back(tfs->make<TH1F>(ss_hist_name.str().c_str(),ss_hist_title.str().c_str(),2000,0,4096));
     //     std::cout << "Created histo. Total histos is now  " << hist_vector_eventNumber.size()<< std::endl;
     for(size_t i_t=0; i_t< rd.Samples(); ++i_t){
       short my_adc_value = rd.ADC(i_t);  
       if(my_adc_value!=0) hist_vector_eventNumber.back()->SetBinContent(i_t+1,my_adc_value);
    }
  }

   timer.Stop();
   std::cout << " time to create histograms " << timer.RealTime()<< std::endl;
   std::cout << " event " << eventNumber<< " is now completed." << std::endl;

   master.Stop();
   std::cout << " The total time for this event is   " << master.RealTime() 
	     << " The overall total time accross all events is " << master.RealTime()*eventNumber << std::endl;
}

DEFINE_ART_MODULE(tpcAnalysis::TPCWavefromCreator)	 
				  
  //fin

