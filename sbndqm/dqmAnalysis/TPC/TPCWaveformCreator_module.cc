#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <sstream>
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

/*
 * Uses the Analysis class to print stuff to file
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
   
};

tpcAnalysis::TPCWavefromCreator::TPCWavefromCreator(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p){
}
  
void tpcAnalysis::TPCWavefromCreator::analyze(art::Event const & evt) {
  art::EventNumber_t eventNumber = evt.id().event();
  //get the raw digits from the event
  art::Handle<std::vector<raw::RawDigit>> raw_digits_handle;
  evt.getByLabel("daq", raw_digits_handle);

   //wes prefers working with vectors, but you can use the handle/pointer if you want
   std::vector<raw::RawDigit> const& raw_digits_vector(*raw_digits_handle);
   std::cout << "Size of vector is " << raw_digits_vector.size() << " or " << raw_digits_handle->size() << std::endl;
 
   art::ServiceHandle<art::TFileService> tfs; 
   std::vector<TH1F*> hist_vector_eventNumber;

   std::cout <<  "Run " << evt.run() << ", subrun " << evt.subRun()
             << ", event " << eventNumber << " has " << raw_digits_handle->size()
	     << " channels"<< std::endl;

   for(size_t i_d=0; i_d<raw_digits_vector.size(); ++i_d){ 
     // using 'auto' here because I'm too lazy to type raw::RawDigit                                                                                  
     const auto& rd = raw_digits_vector[i_d];

     std::cout << "Looking at " << i_d << " rawdigit. Channel number is ... " << rd.Channel() << std::endl;  
     std::stringstream ss_hist_title,ss_hist_name;
     ss_hist_title << "(Run,Event,rawdigit,Channel)"
    		   << evt.run() <<","
		   <<eventNumber << ","
		   <<i_d <<","
		   <<rd.Channel();

     ss_hist_name << "Waveform "
	          <<eventNumber <<"_"
	          <<i_d;

     std::cout << "going to create histogram " << ss_hist_name.str()<< std::endl;


     hist_vector_eventNumber.push_back(tfs->make<TH1F>(ss_hist_name.str().c_str(),ss_hist_title.str().c_str(),2000,0,4096));
     std::cout << "Created histo. Total histos is now  " << hist_vector_eventNumber.size()<< std::endl;
     for(size_t i_t=0; i_t< rd.Samples(); ++i_t){
       short my_adc_value = rd.ADC(i_t);  
       if(my_adc_value!=0) hist_vector_eventNumber.back()->SetBinContent(i_t+1,my_adc_value);
    }
  }


}


DEFINE_ART_MODULE(tpcAnalysis::TPCWavefromCreator)	 
				  
  //fin

