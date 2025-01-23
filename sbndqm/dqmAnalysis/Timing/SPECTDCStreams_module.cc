////////////////////////////////////////////////////////////////////////
//
// SPEC TDC DQM Module (23rd August 2023)
// Lan Nguyen (vclnguyen@ucsb.edu)
// Sabrina Brickner (sabrinabrickner@ucsb.edu)
// 
// The module takes data product DAQTimestamp, output from SPECTDC decoder
// and sends metric to Redis
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDFilter.h"
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
#include "sbndqm/dqmAnalysis/Utils/SBNDHLTFilterUtils.hh"

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

class SPECTDCStreams : public art::EDAnalyzer { 

  public: 

      explicit SPECTDCStreams(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization 
 
      virtual void analyze(art::Event const & evt); 

      // Define your function
      void ResetVars();	
      float OneToOneDiff(std::vector<uint64_t> early_ts, std::vector<uint64_t> late_ts);
      std::vector<float> ManyToOneDiff(std::vector<uint64_t> many_ts, std::vector<uint64_t> one_ts);
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

      float fExpected_BES_CRTT1_diff;
      float fExpected_BES_CRTT1_jitter;

      float fExpected_RWM_BES_diff;
      float fExpected_RWM_BES_jitter;

      float fExpected_ETRIG_BES_diff;
      float fExpected_ETRIG_BES_jitter;

      float fExpected_FTRIG_ETRIG_diff;
      float fExpected_FTRIG_ETRIG_jitter;

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

      float BES_CRTT1_diff; 
      float RWM_BES_diff;  
      float ETRIG_BES_diff;
      std::vector<float> FTRIG_ETRIG_diff;
     
}; 
 

SPECTDCStreams::SPECTDCStreams(fhicl::ParameterSet const & pset) 
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
  , fExpected_BES_CRTT1_diff(pset.get<float>("Expected_BES_CRTT1_diff", 1.5)) //ms
  , fExpected_BES_CRTT1_jitter(pset.get<float>("Expected_BES_CRTT1_jitter", 0.1)) //ms
  , fExpected_RWM_BES_diff(pset.get<float>("Expected_RWM_BES_diff", 2)) //us
  , fExpected_RWM_BES_jitter(pset.get<float>("Expected_RWM_BES_jitter", 1)) //us
  , fExpected_ETRIG_BES_diff(pset.get<float>("Expected_ETRIG_BES_diff", 332)) //us
  , fExpected_ETRIG_BES_jitter(pset.get<float>("Expected_ETRIG_BES_jitter", 1)) //us
  , fExpected_FTRIG_ETRIG_diff(pset.get<float>("Expected_FTRIG_ETRIG_diff", 2)) //ms
  , fExpected_FTRIG_ETRIG_jitter(pset.get<float>("Expected_FTRIG_ETRIG_jitter", 0.5)) //ms
{
  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("SPECTDCMetricConfig"));
} 

void SPECTDCStreams::ResetVars() {
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

bool SPECTDCStreams::CheckSecondRollOver(std::vector<art::Ptr<sbnd::timing::DAQTimestamp>> ts_vec){

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

void SPECTDCStreams::Check_nCRTT1() {
  if (nCRTT1 == 1) std::cout << "Good event has one CRT T1 Reset." << std::endl; 
  if (nCRTT1 != 1) std::cout << "BAD!! n CRT T1 = " << nCRTT1 << ". Expected one!!" << std::endl;
}

void SPECTDCStreams::Check_nBES() {
  if (nBES == 1) std::cout << "Good event has one BES." << std::endl; 
  if (nBES != 1) std::cout << "BAD!! n BES = " << nBES << ". Expected one!!" << std::endl;
}

void SPECTDCStreams::Check_nRWM() {
  if (nRWM == 1) std::cout << "Good event has one RWM." << std::endl; 
  if (nRWM != 1) std::cout << "BAD!! n RWM = " << nRWM << ". Expected one!!" << std::endl;
}

void SPECTDCStreams::Check_nFTRIG() {
  if (nFTRIG > 0 ) std::cout << "Good event has " << nFTRIG << " FTRIG." << std::endl; 
  if (nFTRIG == 0) std::cout << "BAD!! n FTRIG = " << nFTRIG << ". Expected 10~20 FTRIG!!" << std::endl;
}

void SPECTDCStreams::Check_nETRIG() {
  if (nETRIG == 1 ) std::cout << "Good event has one ETRIG." << std::endl; 
  if (nETRIG != 1) std::cout << "BAD!! n ETRIG = " << nETRIG << ". Expected one!!" << std::endl;
}

void SPECTDCStreams::Check_BES_CRTT1_diff() {
  if ( (BES_CRTT1_diff < (fExpected_BES_CRTT1_diff + fExpected_BES_CRTT1_jitter)) 
    && (BES_CRTT1_diff > (fExpected_BES_CRTT1_diff - fExpected_BES_CRTT1_jitter)) ){
    std::cout << std::setprecision(3) << "Good! BES - CRT T1 = " << BES_CRTT1_diff << " ms." << std::endl;
  }else{
    std::cout << std::setprecision(3) << "BAD!! BES - CRT T1 = " << BES_CRTT1_diff << " ms out of expectation of " << fExpected_BES_CRTT1_diff;
    std::cout << "+-" << fExpected_BES_CRTT1_jitter << " ms" << std::endl;
  } 
}

void SPECTDCStreams::Check_RWM_BES_diff() {
  if ( (RWM_BES_diff < (fExpected_RWM_BES_diff + fExpected_RWM_BES_jitter)) 
    && (RWM_BES_diff > (fExpected_RWM_BES_diff - fExpected_RWM_BES_jitter)) ){
    std::cout << std::setprecision(3) << "Good! RWM - BES = " << RWM_BES_diff << " us." << std::endl;
  }else{
    std::cout << std::setprecision(3) << "BAD!! RWM - BES = " << RWM_BES_diff << " us out of expectation of " << fExpected_RWM_BES_diff;
    std::cout << "+-" << fExpected_RWM_BES_jitter << " us" << std::endl;
  } 
}

void SPECTDCStreams::Check_ETRIG_BES_diff() {
  if ( (ETRIG_BES_diff < ( fExpected_ETRIG_BES_diff + fExpected_ETRIG_BES_jitter)) 
    && (ETRIG_BES_diff > ( fExpected_ETRIG_BES_diff - fExpected_ETRIG_BES_jitter)) ){
    std::cout << std::setprecision(3) << "Good! ETRIG - BES = " << ETRIG_BES_diff << " us." << std::endl;
  }else{
    std::cout << std::setprecision(3) << "BAD!! RWM - BES = " << ETRIG_BES_diff << " us out of expectation of " << fExpected_ETRIG_BES_diff;
    std::cout << "+-" << fExpected_ETRIG_BES_jitter << " us" << std::endl;
  } 
}

void SPECTDCStreams::Check_FTRIG_ETRIG_diff() {
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

float SPECTDCStreams::OneToOneDiff(std::vector<uint64_t> early_ts, std::vector<uint64_t> late_ts){

  float diff = DBL_MAX;

  if ((early_ts.size() == 1)  && (late_ts.size() == 1)){

    //get the ns part so it's only 9 digits for float
    float early_ts_nspart = early_ts.back()%uint64_t(1e9);
    float late_ts_nspart = late_ts.back()%uint64_t(1e9);
    
    diff = late_ts_nspart - early_ts_nspart;
  }
  return diff;
}

std::vector<float> SPECTDCStreams::ManyToOneDiff(std::vector<uint64_t> many_ts, std::vector<uint64_t> one_ts){

  std::vector<float> diff;

  if ((one_ts.size() == 1)  && (many_ts.size() > 1)){
    
    //get the ns part so it's only 9 digits for float
    float one_ts_nspart = one_ts.back()%uint64_t(1e9);
    
    for (auto const ts: many_ts){

      //get the ns part so it's only 9 digits for float
      float ts_nspart = ts%uint64_t(1e9);
      diff.push_back(ts_nspart - one_ts_nspart);
    }  
  }
  return diff;
}

void SPECTDCStreams::analyze(art::Event const & e) { 

  //------------------------------------------------------------------------------// 
  //Get event number 
  _event = e.id().event(); 

  if (fVerbose >= 1) std::cout << "================= EVENT " << _event << " =================" << std::endl; 

  //------------------------------------------------------------------------------// 
  // Get DAQTimestamps products 
  art::Handle<std::vector<sbnd::timing::DAQTimestamp>> DAQTimestampHandle; 
  e.getByLabel(fDAQTimestampLabel, DAQTimestampHandle); 

  if( !DAQTimestampHandle.isValid() || DAQTimestampHandle->empty() ){
    mf::LogError("SPECTDCStreams::analyze") << "Data product '" << fDAQTimestampLabel << "' has no timing::DAQTimestamp in it! Skip event " << _event << ".\n"; 
    return; 
  } 

  std::vector<art::Ptr<sbnd::timing::DAQTimestamp>> DAQTimestampVec; 
  art::fill_ptr_vector(DAQTimestampVec, DAQTimestampHandle); 
  if (fVerbose >= 2) std::cout << "timing::DAQTimestamp size = " << DAQTimestampVec.size() << std::endl;
  
  if (CheckSecondRollOver(DAQTimestampVec)){
    if (fVerbose >= 1) std::cout << "Event has the second roll over. Skip event " << _event << ".\n"; 
    return;
  }	

  //------------------------------------------------------------------------------// 
  //Get PTB fragment container
  art::InputTag itag(fDAQLabel, fPTBContainerInstance);
  auto cont_frags = e.getHandle<artdaq::Fragments>(itag);
  
  if(!cont_frags){
    mf::LogError("SPECTDCStreams::analyze") << "Data product '" << fDAQLabel << "' has no " << fPTBContainerInstance << " in it! Skip event " << _event << ".\n";
    return; 
  }
  else{
    for(auto const& cont : *cont_frags){
      artdaq::ContainerFragment contf(cont);                                           
      hlt_vec=sbndqm::SBNDHLTFilterUtils::GetAllHLTs(&contf);

      if (fVerbose >= 2) {
	std::cout << "HLT size = " << hlt_vec.size() << ", contains HLT = ";
	for (auto const hlt: hlt_vec){
          std::cout << hlt << " ";
        }
	std::cout << std::endl;
      }
    }
  }

  if(hlt_vec.size() == 0 ) {
    if (fVerbose >= 1) std::cout << "No HLTs found. Something went wrong. Skip event " << _event << ".\n";
    return;
  }
  //------------------------------------------------------------------------------// 
  //Apply Filter based on HLT, copy from SBNDGayeFilter: https://github.com/SBNSoftware/sbndaq-artdaq/blob/v1_10_03/sbndaq-artdaq/ArtModules/SBND/SBNDGateFilter_module.cc
  //Hardcoded for 3 main streams: beam, offbeam and crossing muons

  bool passBeam = false;
  bool passOffbeam = false;
  bool passXmuon = false;

  passBeam = sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, fHLT_beam, fHLT_beam_excluded);
  passOffbeam = sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, fHLT_offbeam, fHLT_offbeam_excluded);
  passXmuon = sbndqm::SBNDHLTFilterUtils::ApplyGateFilter(hlt_vec, fHLT_crossingmuon, fHLT_crossingmuon_excluded);

  if ((passBeam + passOffbeam + passXmuon) == 0){
    if (fVerbose >= 1) std::cout << "No HLT filter passes! Something went wrong. Skip event " << _event << ".\n";
    return;
  } 
  else if ((passBeam + passOffbeam + passXmuon) != 1) {
    if (fVerbose >= 1) std::cout << "More than 1 filter pass! Something went wrong. Skip event " << _event << ".\n";
    return;
  }
  else{
    if (fVerbose >= 1) std::cout << "One filter passes! ";
    if (passBeam && fVerbose >= 1) std::cout << "Event " << _event << " is beam stream." << std::endl;
    if (passOffbeam && fVerbose >= 1) std::cout << "Event " << _event << " is offbeam stream." << std::endl;
    if (passXmuon && fVerbose >= 1) std::cout << "Event " << _event << " is crossing muon stream." << std::endl;
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

      if (fVerbose >= 2)  std::cout << "   Chan" << ts->Channel() << " " << ts->Name() << " has timestamp " << ts->Timestamp() << " ns and offset " << ts->Offset() << " ns " << std::endl; 
  }
 
  if (fVerbose >= 1){
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

    if (fVerbose >= 1){
      std::cout << std::endl;

      Check_nCRTT1(); Check_nBES(); Check_nRWM(); Check_nFTRIG(); Check_nETRIG();
      
      std::cout << std::endl;

      if ((nBES == 1) & (nCRTT1 == 1)) Check_BES_CRTT1_diff();
      if ((nRWM == 1) & (nBES == 1)) Check_RWM_BES_diff();
      if ((nETRIG == 1) & (nBES == 1)) Check_ETRIG_BES_diff();
      if ((nETRIG == 1) & (ftrig_vec.size() > 1))Check_FTRIG_ETRIG_diff();
      
      std::cout << std::endl;
    }

    //Send metrics
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nCRTT1", nCRTT1, 3, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nBES", nBES, 3, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nRWM", nRWM, 3, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nFTRIG", nFTRIG, 3, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "nETRIG", nETRIG, 3, artdaq::MetricMode::LastPoint);  
  
    if ((nBES == 1) & (nCRTT1 == 1)) sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "BES_CRTT1_diff", BES_CRTT1_diff, 3, artdaq::MetricMode::LastPoint);  
    if ((nRWM == 1) & (nBES == 1)) sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "RWM_BES_diff", RWM_BES_diff, 3, artdaq::MetricMode::LastPoint);  
    if ((nETRIG == 1) & (nBES == 1)) sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_BES_diff", ETRIG_BES_diff, 3, artdaq::MetricMode::LastPoint);  
    if ((nETRIG == 1) & (ftrig_vec.size() > 1)){
      for (auto const ts: FTRIG_ETRIG_diff){
        sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "FTRIG_ETRIG_diff", ts, 3, artdaq::MetricMode::LastPoint); 
      }
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

    if (fVerbose >= 1){
      std::cout << std::endl;

      Check_nCRTT1(); Check_nFTRIG(); Check_nETRIG();
      
      std::cout << std::endl;

      if ((nETRIG == 1) & (ftrig_vec.size() > 1))Check_FTRIG_ETRIG_diff();

      std::cout << std::endl;
    } 

    //Send metrics
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "nCRTT1", nCRTT1, 3, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "nFTRIG", nFTRIG, 3, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "nETRIG", nETRIG, 3, artdaq::MetricMode::LastPoint);  

    if ((nETRIG == 1) & (ftrig_vec.size() > 1)){
      for (auto const ts: FTRIG_ETRIG_diff){
        sbndaq::sendMetric("SPECTDC_Streams_Timing", "1", "FTRIG_ETRIG_diff", ts, 3, artdaq::MetricMode::LastPoint); 
      }
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

    if (fVerbose >= 1){
      std::cout << std::endl;

      Check_nFTRIG(); Check_nETRIG();
      
      std::cout << std::endl;

      if ((nETRIG == 1) & (ftrig_vec.size() > 1))Check_FTRIG_ETRIG_diff();

      std::cout << std::endl;
    } 

    //Send metrics
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "2", "nFTRIG", nFTRIG, 3, artdaq::MetricMode::LastPoint);  
    sbndaq::sendMetric("SPECTDC_Streams_Timing", "2", "nETRIG", nETRIG, 3, artdaq::MetricMode::LastPoint);  

    if ((nETRIG == 1) & (ftrig_vec.size() > 1)){
      for (auto const ts: FTRIG_ETRIG_diff){
        sbndaq::sendMetric("SPECTDC_Streams_Timing", "2", "FTRIG_ETRIG_diff", ts, 3, artdaq::MetricMode::LastPoint); 
      }
    }
    //End of Send metrics
  }
 
  if (fVerbose >= 1) std::cout << "===============================================" << std::endl; 

  ResetVars();
} 
DEFINE_ART_MODULE(SPECTDCStreams) 
