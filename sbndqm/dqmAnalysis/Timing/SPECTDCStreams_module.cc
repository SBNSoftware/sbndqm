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
}

double sbndaq::SPECTDCStreams::OneToOneDiff(std::vector<uint64_t> early_ts, std::vector<uint64_t> late_ts){

  //TODO: Handle timestamp difference when there's a second roll over
  double diff = -999999;

  if ((early_ts.size() == 1)  && (late_ts.size() == 1)){

    //get the ns part so it's only 9 digits for double
    double early_ts_nspart = early_ts.back()%uint64_t(1e9);
    double late_ts_nspart = late_ts.back()%uint64_t(1e9);
    
    diff = late_ts_nspart - early_ts_nspart;
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
  // Send metrics based on HLT

  /*BNBZeroBias + BNBLight Stream
  ch0: CRT T1 reset (from PTB)  -- once per event 
  ch1: BES (Beam Early Signal) -- once per event 
  ch2: RWM (Resistor Wall Monitor) -- once per event 
  ch3: FTRIG (Flash trigger from PTB)  -- multiple per event 
  ch4: ETRIG (Event trigger from PTB)  -- once per event 
  */
  if (passBeam) {

    double BES_CRTT1_diff = OneToOneDiff(crtt1_vec, bes_vec);

    //Expect +2 us
    double RWM_BES_diff = OneToOneDiff(bes_vec, rwm_vec);

    //Expect +332 us
    double ETRIG_BES_diff = OneToOneDiff(bes_vec, etrig_vec);

    // ETRIG - FTRIG 
    //uint64_t ETRIG_RWM_diff = 0;
    //if ((nRWM == 1)  && (nETRIG == 1)){
    //  std::cout << "ETRIG - RWM = ";
    //  std::cout << etrig_vec.back() - rwm_vec.back() << std::endl;
    //}

    if (fVerbose > 0){
      if (nCRTT1 == 1) std::cout << "Good event has one CRT T1 Reset." << std::endl; 
      if (nCRTT1 != 1) std::cout << "BAD!! n CRT T1 = " << nCRTT1 << ". Expected one!!" << std::endl;

      if (nBES == 1) std::cout << "Good event has one BES." << std::endl; 
      if (nBES != 1) std::cout << "BAD!! n BES = " << nBES << ". Expected one!!" << std::endl;

      if (nRWM == 1) std::cout << "Good event has one RWM." << std::endl; 
      if (nRWM != 1) std::cout << "BAD!! n RWM = " << nRWM << ". Expected one!!" << std::endl;

      if (nFTRIG > 0 ) std::cout << "Good event has " << nFTRIG << " FTRIG." << std::endl; 
      if (nFTRIG == 0) std::cout << "BAD!! n FTRIG = " << nFTRIG << ". Expected 10~20 FTRIG!!" << std::endl;
             
      if (nETRIG == 1 ) std::cout << "Good event has one ETRIG." << std::endl; 
      if (nETRIG != 1) std::cout << "BAD!! n ETRIG = " << nETRIG << ". Expected one!!" << std::endl;
      
      std::cout << std::endl;
      
      //Expect +2.5 ms
      int jitter = 0.5;
      if ( (BES_CRTT1_diff < 2.5 + jitter) && (BES_CRTT1_diff > 2.5 - jitter)){
        std::cout << "Good! BES - CRT T1 = " << BES_CRTT1_diff/1'000'000 << " ms." << std::endl;
      }else{
        std::cout << "BAD!! BES - CRT T1 = " << BES_CRTT1_diff/1'000'000 << " ms out of expectation of 2.5 ms +- jitter." << std::endl;
      } 

      std::cout << std::setprecision(9) << "RWM - BES = " << RWM_BES_diff/1'000 << " us." << std::endl;
    
      std::cout << std::setprecision(9) << "ETRIG - BES = " << ETRIG_BES_diff/1'000 << " us." << std::endl;
    
      std::cout << std::endl;
    }

    sbndaq::sendMetric("SPECTDC_STREAMS_Timing", "0", "nCRTT1", nCRTT1, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_STREAMS_Timing", "0", "nBES", nBES, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_STREAMS_Timing", "0", "nRWM", nRWM, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_STREAMS_Timing", "0", "nFTRIG", nFTRIG, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_STREAMS_Timing", "0", "nETRIG", nETRIG, 0, artdaq::MetricMode::LastPoint);  
  }

  /*OffbeamZeroBias + OffbeamLight Stream
  ch0: CRT T1 reset (from PTB) -- once per event 
  ch1: BES (Beam Early Signal) -- once per event 
  ch2: RWM (Resistor Wall Monitor) -- no 
  ch3: FTRIG (Flash trigger from PTB)  -- multiple per event 
  ch4: ETRIG (Event trigger from PTB)  -- once per event 
  */
  if (passOffbeam) {

    if (fVerbose){
      if (nCRTT1 == 1) std::cout << "Good event has one CRT T1 Reset." << std::endl; 
      if (nCRTT1 != 1) std::cout << "BAD!! n CRT T1 = " << nCRTT1 << ". Expected one!!" << std::endl;

      if (nBES == 1) std::cout << "Good event has one BES." << std::endl; 
      if (nBES != 1) std::cout << "BAD!! n BES = " << nBES << ". Expected one!!" << std::endl;

      if (nFTRIG > 0 ) std::cout << "Good event has " << nFTRIG << " FTRIG." << std::endl; 
      if (nFTRIG == 0) std::cout << "BAD!! n FTRIG = " << nFTRIG << ". Expected 10~20 FTRIG!!" << std::endl;
             
      if (nETRIG > 0 ) std::cout << "Good event has " << nETRIG << " ETRIG." << std::endl; 
      if (nETRIG == 0) std::cout << "BAD!! n ETRIG = " << nETRIG << ". Expected 1~2 ETRIG!!" << std::endl;
    } 

    sbndaq::sendMetric("SPECTDC_STREAMS_Timing", "0", "nCRTT1", nCRTT1, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_STREAMS_Timing", "0", "nBES", nBES, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("spectdc_streams_timing", "0", "nFTRIG", nFTRIG, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("spectdc_streams_timing", "0", "nETRIG", nETRIG, 0, artdaq::MetricMode::LastPoint);  
  }

  /*CrossingMuon Stream
  ch0: CRT T1 reset (from PTB) -- maybe 
  ch1: BES (Beam Early Signal) -- maybe 
  ch2: RWM (Resistor Wall Monitor) -- no 
  ch3: FTRIG (Flash trigger from PTB)  -- multiple per event 
  ch4: ETRIG (Event trigger from PTB)  -- once per event 
  */
  if (passXmuon) {

    if (fVerbose){
      if (nFTRIG > 0 ) std::cout << "Good event has " << nFTRIG << " FTRIG." << std::endl; 
      if (nFTRIG == 0) std::cout << "BAD!! n FTRIG = " << nFTRIG << ". Expected 10~20 FTRIG!!" << std::endl;
             
      if (nETRIG > 0 ) std::cout << "Good event has " << nETRIG << " ETRIG." << std::endl; 
      if (nETRIG == 0) std::cout << "BAD!! n ETRIG = " << nETRIG << ". Expected 1~2 ETRIG!!" << std::endl;
    } 

    sbndaq::sendMetric("spectdc_streams_timing", "0", "nFTRIG", nFTRIG, 0, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("spectdc_streams_timing", "0", "nETRIG", nETRIG, 0, artdaq::MetricMode::LastPoint);  
  }
  
  //sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "RWM_BES_const", RWM_BES_const, 0, artdaq::MetricMode::LastPoint);  
  //sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "CRT_BES_const", CRT_BES_const, 0, artdaq::MetricMode::LastPoint);  

  //sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_BES_diff", ETRIG_BES_diff, 0, artdaq::MetricMode::LastPoint);  
  //sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_RWM_diff", ETRIG_RWM_diff, 0, artdaq::MetricMode::LastPoint);  
  //sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_FTRIG_diff", ETRIG_FTRIG_diff, 0, artdaq::MetricMode::LastPoint);  
  //sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "BES_FTRIG_diff", BES_FTRIG_diff, 0, artdaq::MetricMode::LastPoint); 
 
  if (fVerbose > 0) std::cout << "===============================================" << std::endl; 

  ResetVars();
} 
DEFINE_ART_MODULE(sbndaq::SPECTDCStreams) 
