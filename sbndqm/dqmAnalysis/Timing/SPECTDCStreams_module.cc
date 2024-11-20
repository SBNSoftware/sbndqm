////////////////////////////////////////////////////////////////////////
//
// SPEC TDC DQM Module (23rd August 2023)
// Lan Nguyen (vclnguyen1@sheffield.ac.uk)
// Sabrina Brickner (sabrinabrickner@ucsb.edu)
// 
// The module takes data product DAQTimestamp, output from SPECTDC decoder
// and sends metric to Redis
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art_root_io/TFileService.h"
#include "canvas/Persistency/Common/FindManyP.h"

#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"
#include "sbnobj/SBND/Timing/DAQTimestamp.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/PTBFragment.hh"

#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"

#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>

#include "messagefacility/MessageLogger/MessageLogger.h"

namespace sbndaq { 
 
  class SPECTDCStreams : public art::EDAnalyzer { 
 
    public: 
 
        explicit SPECTDCStreams(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization 
   
        virtual void analyze(art::Event const & evt); 
  
        // Define your function
	std::vector<uint64_t> GetAllHLTs(artdaq::ContainerFragment *trigfrag);
	std::vector<uint64_t> GetHLT(sbndaq::CTBFragment ptb_fragment);
        bool ApplyGateFilter(std::vector<uint64_t> triggers, std::vector<uint64_t> ftrigger_type, std::vector<uint64_t> fexcluded_trigger);
	void ResetVars();	
        double OneToOneDiff(std::vector<uint64_t> early_ts, std::vector<uint64_t> late_ts);
        std::vector<double> ManyToOneDiff(std::vector<uint64_t> many_ts, std::vector<uint64_t> one_ts);
	bool CheckSecondRollOver(std::vector<art::Ptr<sbnd::timing::DAQTimestamp>> ts_vec);	

	void Check_nCRTT1();
	void Check_nBES();
	void Check_nRWM();
	void Check_nFTRIG();
	void Check_nETRIG();

	void Check_BES_CRTT1_diff();
	void Check_RWM_BES_diff();
	void Check_ETRIG_BES_diff();
	void Check_FTRIG_ETRIG_diff();

    private: 
         
        //Fcl Config 
        std::string fDAQTimestampLabel;
	std::string fDAQLabel;
	std::string fPTBContainerInstance;
	
	int fVerbose;

	std::vector<uint64_t> fHLT_beam;
	std::vector<uint64_t> fHLT_beam_excluded;
	std::vector<uint64_t> fHLT_offbeam;
	std::vector<uint64_t> fHLT_offbeam_excluded;
	std::vector<uint64_t> fHLT_crossingmuon;
	std::vector<uint64_t> fHLT_crossingmuon_excluded;

        uint32_t fCRTT1_ch;
        uint32_t fBES_ch;
        uint32_t fRWM_ch;
        uint32_t fFTRIG_ch;
        uint32_t fETRIG_ch;

        double fExpected_BES_CRTT1_diff;
        double fExpected_BES_CRTT1_jitter;

        double fExpected_RWM_BES_diff;
        double fExpected_RWM_BES_jitter;

        double fExpected_ETRIG_BES_diff;
        double fExpected_ETRIG_BES_jitter;

        double fExpected_FTRIG_ETRIG_diff;
        double fExpected_FTRIG_ETRIG_jitter;

        //Class variables 
        int _event; 
	
	std::vector<uint64_t> hlt_vec;

        std::vector<uint64_t> crtt1_vec; 
        std::vector<uint64_t> bes_vec; 
        std::vector<uint64_t> rwm_vec; 
        std::vector<uint64_t> ftrig_vec; 
        std::vector<uint64_t> etrig_vec; 

        int nCRTT1 = 0; 
        int nBES = 0; 
        int nRWM = 0; 
        int nFTRIG = 0; 
        int nETRIG = 0; 

	double BES_CRTT1_diff; 
	double RWM_BES_diff;  
	double ETRIG_BES_diff;
	std::vector<double> FTRIG_ETRIG_diff;
       
    }; 
} 

sbndaq::SPECTDCStreams::SPECTDCStreams(fhicl::ParameterSet const & pset) 
  : EDAnalyzer(pset) 
  , fDAQTimestampLabel(pset.get<std::string>("DAQTimestampLabel", "daqSPECTDC")) 
  , fDAQLabel(pset.get<std::string>("DAQLabel", "daq"))
  , fPTBContainerInstance(pset.get<std::string>("PTBContainerInstance", "ContainerPTB")) 
  , fVerbose(pset.get<int>("Verbose", 0))
  , fHLT_beam(pset.get<std::vector<uint64_t>>("HLT_beam"))
  , fHLT_beam_excluded(pset.get<std::vector<uint64_t>>("HLT_beam_excluded"))
  , fHLT_offbeam(pset.get<std::vector<uint64_t>>("HLT_offbeam"))
  , fHLT_offbeam_excluded(pset.get<std::vector<uint64_t>>("HLT_offbeam_excluded"))
  , fHLT_crossingmuon(pset.get<std::vector<uint64_t>>("HLT_crossingmuon"))
  , fHLT_crossingmuon_excluded(pset.get<std::vector<uint64_t>>("HLT_crossingmuon_excluded"))
  , fCRTT1_ch(pset.get<uint32_t>("CRTT1_ch", 0))
  , fBES_ch(pset.get<uint32_t>("BES_ch", 1))
  , fRWM_ch(pset.get<uint32_t>("RWM_ch", 2))
  , fFTRIG_ch(pset.get<uint32_t>("FTRIG_ch", 3))
  , fETRIG_ch(pset.get<uint32_t>("ETRIG_ch", 4))
  , fExpected_BES_CRTT1_diff(pset.get<double>("Expected_BES_CRTT1_diff", 2.5)) //ms
  , fExpected_BES_CRTT1_jitter(pset.get<double>("Expected_BES_CRTT1_jitter", 0.1)) //ms
  , fExpected_RWM_BES_diff(pset.get<double>("Expected_RWM_BES_diff", 2)) //us
  , fExpected_RWM_BES_jitter(pset.get<double>("Expected_RWM_BES_jitter", 1)) //us
  , fExpected_ETRIG_BES_diff(pset.get<double>("Expected_ETRIG_BES_diff", 332)) //us
  , fExpected_ETRIG_BES_jitter(pset.get<double>("Expected_ETRIG_BES_jitter", 1)) //us
  , fExpected_FTRIG_ETRIG_diff(pset.get<double>("Expected_FTRIG_ETRIG_diff", 2)) //ms
  , fExpected_FTRIG_ETRIG_jitter(pset.get<double>("Expected_FTRIG_ETRIG_jitter", 0.5)) //ms
{
  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("SPECTDCMetricConfig"));
} 

std::vector<uint64_t> sbndaq::SPECTDCStreams::GetAllHLTs(artdaq::ContainerFragment*  ptb_container_fragment){
  std::vector<uint64_t> triggers;    

  for (size_t f=0; f<ptb_container_fragment->block_count(); ++f){//loop over container of fragments
    //std::cout << "hello" << std::endl;
    artdaq::Fragment frag=*ptb_container_fragment->at(f).get();
    sbndaq::CTBFragment ptb_fragment(frag);
    //==============
    std::vector fragTriggers=GetHLT(ptb_fragment); //Not sure that there could really be multiple HLTs in a single fragment but just in case I'll make it vector
    triggers.insert(triggers.end(), fragTriggers.begin(), fragTriggers.end() );//append list of triggers in this fragment to all of the HLTs in the container
  }//end loop over fragments
  
  return triggers;
}

std::vector<uint64_t> sbndaq::SPECTDCStreams::GetHLT(sbndaq::CTBFragment ptb_fragment){
  std::vector<uint64_t> triggers;  

  for ( size_t i = 0; i < ptb_fragment.NWords(); i++ ) {//loop over words in fragment       
    if  (ptb_fragment.Word(i)->word_type !=0x2 ) continue; //0x2 is the type for an HLT (0x1 for LLT) 
    uint64_t hlt_mask = ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF;
    // Process each set bit in hlt_mask as a separate HLT trigger
    while (hlt_mask) {
            uint64_t hlttrigger = __builtin_ctzll(hlt_mask); // Find the least significant set bit
            hlt_mask &= (hlt_mask - 1); // Clear the least significant set bit
            if (hlttrigger >= 20) continue;  //HLT triggers greater then 20 are reserved for non event triggers
            triggers.emplace_back(hlttrigger);
    }
  }
  
  return triggers;
}

bool sbndaq::SPECTDCStreams::ApplyGateFilter(std::vector<uint64_t> triggers, std::vector<uint64_t> ftrigger_type, std::vector<uint64_t> fexcluded_triggers)
{

  std::string trigTypeList="{";//create list of the selected trigger types
  for ( auto const trigger_type: ftrigger_type ){//get a string with the list of all the trigger types we're looking for 
    trigTypeList+=std::to_string(trigger_type)+",";
  }
  trigTypeList+="}";

  if ( triggers.size() > 0 )  if ( fVerbose > 1 ) std::cout << "This event has " << triggers.size() << " HLTs in it. Filter will pass if any are trigger type == "<< trigTypeList.c_str() << ".\n";

  std::string trigstring="";
  bool passesFilter=false;

  for(auto const hlttrigger: triggers){//Loop over the HLTs found in the fragment
    if(fexcluded_triggers.size()>0){//Need to loop through all of the excluded trigger types to check for them for each present HLT 
      for(auto const excluded_trigger : fexcluded_triggers){//loop over the list of triggers to exclude from the fcl
	if (hlttrigger==excluded_trigger){
	
	  if(fVerbose > 1 ) std::cout << "This Event contains an excluded trigger type " << hlttrigger << " == " << excluded_trigger<< " so fails the filter.\n";
	  return false;
	}
      }
    }//end loop over excluded triggers
    for(auto const trigger_type: ftrigger_type){//loop over the list of triggers to include from the fcl
      if( hlttrigger==trigger_type){

	if (fVerbose > 1) std::cout << "This Event has trigger type " << hlttrigger << "==" << trigger_type << "  and passes filter.\n";

        passesFilter=true;

	if(fexcluded_triggers.size()==0) break;//no need to keep looking through the triggers if none are excluded
      }
      trigstring+= std::to_string(hlttrigger)+", ";
    }
  }//end loop hlts 

  if (passesFilter) return true;
  
  
  if (fVerbose > 1) std::cout << "This Event has trigger type { " << trigstring  << "} == " << trigTypeList.c_str() << " and fails filter.\n";
  return false;

}

void sbndaq::SPECTDCStreams::ResetVars() {
  hlt_vec.clear();
  crtt1_vec.clear();
  bes_vec.clear();
  rwm_vec.clear();
  ftrig_vec.clear();
  etrig_vec.clear();

  nCRTT1 = 0; 
  nBES = 0; 
  nRWM = 0; 
  nFTRIG = 0; 
  nETRIG = 0; 

  BES_CRTT1_diff = DBL_MAX; 
  RWM_BES_diff = DBL_MAX;  
  ETRIG_BES_diff = DBL_MAX;
  FTRIG_ETRIG_diff.clear();
}

bool sbndaq::SPECTDCStreams::CheckSecondRollOver(std::vector<art::Ptr<sbnd::timing::DAQTimestamp>> ts_vec){

  uint64_t earliest_ts = std::numeric_limits<uint64_t>::max();
  uint64_t latest_ts = std::numeric_limits<uint64_t>::max();

  for(unsigned int i = 0; i < ts_vec.size(); i++) { 
    uint64_t ts = ts_vec[i]->Timestamp() + ts_vec[i]->Offset();

    if(i == 0){
      earliest_ts = ts;
      latest_ts = ts;
    }else{
      if (ts < earliest_ts) earliest_ts = ts;
      if (ts > latest_ts) latest_ts = ts;
    }
  }

  //Get the second part
  earliest_ts /= uint64_t(1e9);
  latest_ts /= uint64_t(1e9);
  
  if ((latest_ts - earliest_ts) > 1) return true; 
 
  return false;  
}	

void sbndaq::SPECTDCStreams::Check_nCRTT1() {
  if (nCRTT1 == 1) std::cout << "Good event has one CRT T1 Reset." << std::endl; 
  if (nCRTT1 != 1) std::cout << "BAD!! n CRT T1 = " << nCRTT1 << ". Expected one!!" << std::endl;
}

void sbndaq::SPECTDCStreams::Check_nBES() {
  if (nBES == 1) std::cout << "Good event has one BES." << std::endl; 
  if (nBES != 1) std::cout << "BAD!! n BES = " << nBES << ". Expected one!!" << std::endl;
}

void sbndaq::SPECTDCStreams::Check_nRWM() {
  if (nRWM == 1) std::cout << "Good event has one RWM." << std::endl; 
  if (nRWM != 1) std::cout << "BAD!! n RWM = " << nRWM << ". Expected one!!" << std::endl;
}

void sbndaq::SPECTDCStreams::Check_nFTRIG() {
  if (nFTRIG > 0 ) std::cout << "Good event has " << nFTRIG << " FTRIG." << std::endl; 
  if (nFTRIG == 0) std::cout << "BAD!! n FTRIG = " << nFTRIG << ". Expected 10~20 FTRIG!!" << std::endl;
}

void sbndaq::SPECTDCStreams::Check_nETRIG() {
  if (nETRIG == 1 ) std::cout << "Good event has one ETRIG." << std::endl; 
  if (nETRIG != 1) std::cout << "BAD!! n ETRIG = " << nETRIG << ". Expected one!!" << std::endl;
}

void sbndaq::SPECTDCStreams::Check_BES_CRTT1_diff() {
  if ( (BES_CRTT1_diff < (fExpected_BES_CRTT1_diff + fExpected_BES_CRTT1_jitter)) 
    && (BES_CRTT1_diff > (fExpected_BES_CRTT1_diff - fExpected_BES_CRTT1_jitter)) ){
    std::cout << std::setprecision(3) << "Good! BES - CRT T1 = " << BES_CRTT1_diff << " ms." << std::endl;
  }else{
    std::cout << std::setprecision(3) << "BAD!! BES - CRT T1 = " << BES_CRTT1_diff << " ms out of expectation of " << fExpected_BES_CRTT1_diff;
    std::cout << "+-" << fExpected_BES_CRTT1_jitter << " ms" << std::endl;
  } 
}

void sbndaq::SPECTDCStreams::Check_RWM_BES_diff() {
  if ( (RWM_BES_diff < (fExpected_RWM_BES_diff + fExpected_RWM_BES_jitter)) 
    && (RWM_BES_diff > (fExpected_RWM_BES_diff - fExpected_RWM_BES_jitter)) ){
    std::cout << std::setprecision(3) << "Good! RWM - BES = " << RWM_BES_diff << " us." << std::endl;
  }else{
    std::cout << std::setprecision(3) << "BAD!! RWM - BES = " << RWM_BES_diff << " us out of expectation of " << fExpected_RWM_BES_diff;
    std::cout << "+-" << fExpected_RWM_BES_jitter << " us" << std::endl;
  } 
}

void sbndaq::SPECTDCStreams::Check_ETRIG_BES_diff() {
  if ( (ETRIG_BES_diff < ( fExpected_ETRIG_BES_diff + fExpected_ETRIG_BES_jitter)) 
    && (ETRIG_BES_diff > ( fExpected_ETRIG_BES_diff - fExpected_ETRIG_BES_jitter)) ){
    std::cout << std::setprecision(3) << "Good! ETRIG - BES = " << ETRIG_BES_diff << " us." << std::endl;
  }else{
    std::cout << std::setprecision(3) << "BAD!! RWM - BES = " << ETRIG_BES_diff << " us out of expectation of " << fExpected_ETRIG_BES_diff;
    std::cout << "+-" << fExpected_ETRIG_BES_jitter << " us" << std::endl;
  } 
}

void sbndaq::SPECTDCStreams::Check_FTRIG_ETRIG_diff() {
  std::cout << "Checking " << nFTRIG << " FTRIGs agreement with ETRIG." << std::endl;
  for (auto const ts: FTRIG_ETRIG_diff){
    if ( (ts < (fExpected_FTRIG_ETRIG_diff + fExpected_FTRIG_ETRIG_jitter)) 
    && (ts > (fExpected_FTRIG_ETRIG_diff*-1 - fExpected_FTRIG_ETRIG_jitter)) ){
      std::cout << std::setprecision(3) << "   Good! FTRIG - ETRIG = " << ts << " ms." << std::endl;
    }else{
      std::cout << std::setprecision(3) << "BAD!! FTRIG - ETRIG = " << ts << " ms out of expectation of " << "[-" << fExpected_FTRIG_ETRIG_diff << ", " << fExpected_FTRIG_ETRIG_diff <<"]";
      std::cout << "+-" << fExpected_FTRIG_ETRIG_jitter << " ms" << std::endl;
    } 
  }
}

double sbndaq::SPECTDCStreams::OneToOneDiff(std::vector<uint64_t> early_ts, std::vector<uint64_t> late_ts){

  double diff = DBL_MAX;

  if ((early_ts.size() == 1)  && (late_ts.size() == 1)){

    //get the ns part so it's only 9 digits for double
    double early_ts_nspart = early_ts.back()%uint64_t(1e9);
    double late_ts_nspart = late_ts.back()%uint64_t(1e9);
    
    diff = late_ts_nspart - early_ts_nspart;
  }
  return diff;
}

std::vector<double> sbndaq::SPECTDCStreams::ManyToOneDiff(std::vector<uint64_t> many_ts, std::vector<uint64_t> one_ts){

  std::vector<double> diff;

  if ((one_ts.size() == 1)  && (many_ts.size() > 1)){
    
    //get the ns part so it's only 9 digits for double
    double one_ts_nspart = one_ts.back()%uint64_t(1e9);
    
    for (auto const ts: many_ts){

      //get the ns part so it's only 9 digits for double
      double ts_nspart = ts%uint64_t(1e9);
      diff.push_back(ts_nspart - one_ts_nspart);
    }  
  }
  return diff;
}

void sbndaq::SPECTDCStreams::analyze(art::Event const & e) { 

  //------------------------------------------------------------------------------// 
  //Get event number 
  _event = e.id().event(); 

  if (fVerbose > 0) std::cout << "================= EVENT " << _event << " =================" << std::endl; 

  //------------------------------------------------------------------------------// 
  //Get PTB fragment container
  art::InputTag itag(fDAQLabel, fPTBContainerInstance);
  auto cont_frags = e.getHandle<artdaq::Fragments>(itag);
  
  if(!cont_frags){
    mf::LogError("sbndaq::SPECTDCStreams::analyze") << "Data product '" << fDAQLabel << "' has no " << fPTBContainerInstance << " in it! Skip this event.\n";
    return; 
  }
  else{
    for(auto const& cont : *cont_frags){
      artdaq::ContainerFragment contf(cont);                                           
      hlt_vec=GetAllHLTs(&contf);

      if (fVerbose > 1) {
	std::cout << "HLT size = " << hlt_vec.size() << ", contains HLT = ";
	for (auto const hlt: hlt_vec){
          std::cout << hlt << " ";
        }
	std::cout << std::endl;
      }
    }
  }

  //------------------------------------------------------------------------------// 
  //Apply Filter based on HLT, copy from SBNDGayeFilter: https://github.com/SBNSoftware/sbndaq-artdaq/blob/v1_10_03/sbndaq-artdaq/ArtModules/SBND/SBNDGateFilter_module.cc
  //Hardcoded for 3 main streams: beam, offbeam and crossing muons

  bool passBeam = false;
  bool passOffbeam = false;
  bool passXmuon = false;

  passBeam = ApplyGateFilter(hlt_vec, fHLT_beam, fHLT_beam_excluded);
  passOffbeam = ApplyGateFilter(hlt_vec, fHLT_offbeam, fHLT_offbeam_excluded);
  passXmuon = ApplyGateFilter(hlt_vec, fHLT_crossingmuon, fHLT_crossingmuon_excluded);

  if ((passBeam + passOffbeam + passXmuon) == 0){
    if (fVerbose > 0) std::cout << "No filter passes! Something went wrong. Skip this event." << std::endl;
    return;
  } 
  else if ((passBeam + passOffbeam + passXmuon) != 1) {
    if (fVerbose > 0) std::cout << "More than 1 filter pass! Something went wrong. Skip this event." << std::endl;
    return;
  }
  else{
    std::cout << "One filter passes! ";
    if (passBeam) std::cout << "Event is beam stream." << std::endl;
    if (passOffbeam) std::cout << "Event is offbeam stream." << std::endl;
    if (passXmuon) std::cout << "Event is crossing muon stream." << std::endl;
  }
  //------------------------------------------------------------------------------// 
  // Get DAQTimestamps products 
  art::Handle<std::vector<sbnd::timing::DAQTimestamp>> DAQTimestampHandle; 
  e.getByLabel(fDAQTimestampLabel, DAQTimestampHandle); 

  if( !DAQTimestampHandle.isValid() || DAQTimestampHandle->empty() ){
    mf::LogError("sbndaq::SPECTDCStreams::analyze") << "Data product '" << fDAQTimestampLabel << "' has no timing::DAQTimestamp in it! Skip this event.\n"; 
    return; 
  } 

  std::vector<art::Ptr<sbnd::timing::DAQTimestamp>> DAQTimestampVec; 
  art::fill_ptr_vector(DAQTimestampVec, DAQTimestampHandle); 
  if (fVerbose > 1) std::cout << "timing::DAQTimestamp size = " << DAQTimestampVec.size() << std::endl;
  
  if (CheckSecondRollOver(DAQTimestampVec)){
    if (fVerbose > 0) std::cout << "Event has the second roll over. Skip this event." << std::endl;
    return;
  }	
  //------------------------------------------------------------------------------// 
  // Fill SPECTDC variables  
  
  // Populate local vectors and count channels  
  for(auto const ts: DAQTimestampVec) { 
       
      if (ts->Channel() == fCRTT1_ch) { 
        crtt1_vec.push_back(ts->Timestamp() + ts->Offset()); 
        nCRTT1++; 
      }  
       
      if (ts->Channel() == fBES_ch) { 
        bes_vec.push_back(ts->Timestamp() + ts->Offset()); 
        nBES++; 
      }  
       
      if (ts->Channel() == fRWM_ch) { 
        rwm_vec.push_back(ts->Timestamp() + ts->Offset()); 
        nRWM++; 
      }  
       
      if (ts->Channel() == fFTRIG_ch) { 
        ftrig_vec.push_back(ts->Timestamp() + ts->Offset()); 
        nFTRIG++; 
      }  
       
      if (ts->Channel() == fETRIG_ch) { 
        etrig_vec.push_back(ts->Timestamp() + ts->Offset()); 
        nETRIG++;  
      }

      if (fVerbose > 2)  std::cout << "   Chan" << ts->Channel() << " " << ts->Name() << " has timestamp " << ts->Timestamp() << " ns and offset " << ts->Offset() << " ns " << std::endl; 
  }
 
  if (fVerbose > 1){
    std::cout << "nCRTT1 = " << nCRTT1; 
    std::cout << ", nBES = " << nBES; 
    std::cout << ", nRWM = " << nRWM; 
    std::cout << ", nFTRIG = " << nFTRIG; 
    std::cout << ", nETRIG = " << nETRIG << std::endl; 
   }

  //------------------------------------------------------------------------------// 
  // Send metrics based on HLT. Currently there are 3 main streams: beam, offbeam and crossing muons

  /*BNBZeroBias + BNBLight Stream
  ch0: CRT T1 reset (from PTB)  -- once per event 
  ch1: BES (Beam Early Signal) -- once per event 
  ch2: RWM (Resistor Wall Monitor) -- once per event 
  ch3: FTRIG (Flash trigger from PTB)  -- multiple per event 
  ch4: ETRIG (Event trigger from PTB)  -- once per event 
  */
  if (passBeam) {

    BES_CRTT1_diff = OneToOneDiff(crtt1_vec, bes_vec)/1'000'000; //ns to ms
    RWM_BES_diff = OneToOneDiff(bes_vec, rwm_vec)/1'000; //ns to us
    ETRIG_BES_diff = OneToOneDiff(bes_vec, etrig_vec)/1'000; //ns to us
    
    FTRIG_ETRIG_diff = ManyToOneDiff(ftrig_vec, etrig_vec);
    for (unsigned int i = 0; i < FTRIG_ETRIG_diff.size(); i++){
      FTRIG_ETRIG_diff[i] /= 1'000'000; //ns to ms
    } 

    if (fVerbose > 0){
      std::cout << std::endl;

      Check_nCRTT1();
      Check_nBES();
      Check_nRWM();
      Check_nFTRIG();
      Check_nETRIG();
      
      std::cout << std::endl;

      Check_BES_CRTT1_diff();
      Check_RWM_BES_diff();
      Check_ETRIG_BES_diff();
      Check_FTRIG_ETRIG_diff();
      
      std::cout << std::endl;
    }

    //Send metrics
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nCRTT1", nCRTT1, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nBES", nBES, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nRWM", nRWM, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nFTRIG", nFTRIG, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nETRIG", nETRIG, 0, artdaq::MetricMode::LastPoint);  
  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "BES_CRTT1_diff", BES_CRTT1_diff, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "RWM_BES_diff", RWM_BES_diff, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_BES_diff", ETRIG_BES_diff, 0, artdaq::MetricMode::LastPoint);  
    for (auto const ts: FTRIG_ETRIG_diff){
      sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "FTRIG_ETRIG_diff", ts, 0, artdaq::MetricMode::LastPoint); 
    }
    //End of Send metrics
  }

  /*OffbeamZeroBias + OffbeamLight Stream
  ch0: CRT T1 reset (from PTB) -- once per event 
  ch1: BES (Beam Early Signal) -- no (TDC does not see offbeam BES) 
  ch2: RWM (Resistor Wall Monitor) -- no 
  ch3: FTRIG (Flash trigger from PTB)  -- multiple per event 
  ch4: ETRIG (Event trigger from PTB)  -- once per event 
  */
  if (passOffbeam) {
    
    FTRIG_ETRIG_diff = ManyToOneDiff(ftrig_vec, etrig_vec);
    for (unsigned int i = 0; i < FTRIG_ETRIG_diff.size(); i++){
      FTRIG_ETRIG_diff[i] /= 1'000'000; //ns to ms
    } 

    if (fVerbose){
      std::cout << std::endl;

      Check_nCRTT1();
      Check_nFTRIG();
      Check_nETRIG();
      
      std::cout << std::endl;

      Check_FTRIG_ETRIG_diff();

      std::cout << std::endl;
    } 

    //Send metrics
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "nCRTT1", nCRTT1, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "nFTRIG", nFTRIG, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "nETRIG", nETRIG, 0, artdaq::MetricMode::LastPoint);  

    for (auto const ts: FTRIG_ETRIG_diff){
      sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "FTRIG_ETRIG_diff", ts, 0, artdaq::MetricMode::LastPoint); 
    }
    //End of Send metrics
  }

  /*CrossingMuon Stream
  ch0: CRT T1 reset (from PTB) -- maybe 
  ch1: BES (Beam Early Signal) -- maybe 
  ch2: RWM (Resistor Wall Monitor) -- no 
  ch3: FTRIG (Flash trigger from PTB)  -- multiple per event, although should half compared to beam/offbeam streams
  ch4: ETRIG (Event trigger from PTB)  -- once per event 
  */
  if (passXmuon) {

    FTRIG_ETRIG_diff = ManyToOneDiff(ftrig_vec, etrig_vec);
    for (unsigned int i = 0; i < FTRIG_ETRIG_diff.size(); i++){
      FTRIG_ETRIG_diff[i] /= 1'000'000; //ns to ms
    } 

    if (fVerbose){
      std::cout << std::endl;

      Check_nFTRIG();
      Check_nETRIG();
      
      std::cout << std::endl;

      Check_FTRIG_ETRIG_diff();

      std::cout << std::endl;
    } 

    //Send metrics
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "2", "nFTRIG", nFTRIG, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "2", "nETRIG", nETRIG, 0, artdaq::MetricMode::LastPoint);  

    for (auto const ts: FTRIG_ETRIG_diff){
      sbndaq::sendMetric("SPECTDC_Streams_Timing", "2", "FTRIG_ETRIG_diff", ts, 0, artdaq::MetricMode::LastPoint); 
    }
    //End of Send metrics
  }
 
  if (fVerbose > 0) std::cout << "===============================================" << std::endl; 

  ResetVars();
} 
DEFINE_ART_MODULE(sbndaq::SPECTDCStreams) 
