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
  
    private: 
 
        // Redis Variables (TODO: UnComment) 
         std::string m_redis_hostname; 
         int m_redis_port; 
         fhicl::ParameterSet m_metric_config; 
         
        //Label 
        std::string m_DAQTimestamp_label; 
 
        //Define your variables         
        int _event; 
        int run_num = 0; 
        double GLOB_RWM_BES_diff = 1e20; 
        double GLOB_CRT_BES_diff = 1e20; 
        double GLOB_ETRIG_BES_diff = 1e20; 
        double GLOB_ETRIG_RWM_diff = 1e20;
        double GLOB_ETRIG_FTRIG_diff = 1e20;
        double GLOB_BES_FTRIG_diff = 1e20;


        std::vector<uint64_t>    GLOB_RWM_BES_diff_vec; 
        std::vector<uint64_t>    GLOB_CRT_BES_diff_vec; 
        std::vector<uint64_t>    GLOB_ETRIG_BES_diff_vec;
        std::vector<uint64_t>    GLOB_ETRIG_RWM_diff_vec;
        std::vector<uint64_t>    GLOB_ETRIG_FTRIG_diff_vec;
        std::vector<uint64_t>    GLOB_BES_FTRIG_diff_vec;
 
        //Group by channel 0 
        std::vector<uint64_t>    _tdc_timestamp0; 
        std::vector<std::string> _tdc_name0; 
         
        //Group by channel 1  
        std::vector<uint64_t>    _tdc_timestamp1; 
        std::vector<std::string> _tdc_name1; 
         
        //Group by channel 2  
        std::vector<uint64_t>    _tdc_timestamp2; 
        std::vector<std::string> _tdc_name2; 
         
        //Group by channel 3  
        std::vector<uint64_t>    _tdc_timestamp3; 
        std::vector<std::string> _tdc_name3; 
         
        //Group by channel 4  
        std::vector<uint64_t>    _tdc_timestamp4; 
        std::vector<std::string> _tdc_name4; 
       
    }; 
} 


sbndaq::SPECTDCStreams::SPECTDCStreams(fhicl::ParameterSet const & pset) 
  : EDAnalyzer(pset) 
  // Redis Variables (TODO: UnComment) 
   , m_redis_hostname{ pset.get<std::string>("RedisHostname", "sbnd-db") } 
   , m_redis_port{ pset.get<int>("RedisPort", 6379) } 
   , m_metric_config{ pset.get<fhicl::ParameterSet>("SPECTDCMetricConfig") } 
  , m_DAQTimestamp_label{ pset.get<std::string>("DAQTimestampLabel") } 

{
  
  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("SPECTDCMetricConfig"));

} 
 
void sbndaq::SPECTDCStreams::analyze(art::Event const & e) { 

  //Define metric for Redis (TODO: UnComment)  
//  int level = 3;  
//  artdaq::MetricMode mode = artdaq::MetricMode::Average; 
//  artdaq::MetricMode rate = artdaq::MetricMode::Rate; 
  std::string groupName = "SPECTDC"; 

  //------------------------------------------------------------------------------// 
  //Get event number 
  _event = e.id().event(); 

  // Get DAQTimestamps 
  art::Handle<std::vector<sbnd::timing::DAQTimestamp>> DAQTimestampHandle; 
  e.getByLabel(m_DAQTimestamp_label, DAQTimestampHandle); 
  if(!DAQTimestampHandle.isValid()){ 
    mf::LogError("sbndaq::SPECTDCStreams::analyze")  
         << "Data product '" << m_DAQTimestamp_label << "' has no timing::DAQTimestamp in it!\n"; 
    std::cout 
        << "Data product '" << m_DAQTimestamp_label << "' has no timing::DAQTimestamp in it!\n"; 
     
    //TODO: Should be some check here for DQM if an event has no timestamps from SPECTDC  
  } 
  std::vector<art::Ptr<sbnd::timing::DAQTimestamp>> DAQTimestampVec; 
  art::fill_ptr_vector(DAQTimestampVec, DAQTimestampHandle); 

  // Fill SPECTDC variables to local vector for doing metric maths 
  unsigned nDAQTimestamps = DAQTimestampVec.size(); 
  unsigned nch0 = 0; 
  unsigned nch1 = 0; 
  unsigned nch2 = 0; 
  unsigned nch3 = 0; 
  unsigned nch4 = 0; 
  
  // Populate local vectors and count channels  
  for(unsigned i = 0; i < nDAQTimestamps; ++i) { 
      auto ts = DAQTimestampVec[i]; 
       
      if (ts->Channel() == 0) { 
        _tdc_timestamp0.push_back(ts->Timestamp() + ts->Offset()); 
        _tdc_name0.push_back(ts->Name()); 
        nch0++; 
      }  
       
      if (ts->Channel() == 1) { 
        _tdc_timestamp1.push_back(ts->Timestamp() + ts->Offset()); 
        _tdc_name1.push_back(ts->Name()); 
        nch1++; 
      }  
       
      if (ts->Channel() == 2) { 
        _tdc_timestamp2.push_back(ts->Timestamp() + ts->Offset()); 
        _tdc_name2.push_back(ts->Name()); 
        nch2++; 
      }  
       
      if (ts->Channel() == 3) { 
        _tdc_timestamp3.push_back(ts->Timestamp() + ts->Offset()); 
        _tdc_name3.push_back(ts->Name()); 
        nch3++; 
      }  
       
      if (ts->Channel() == 4) { 
        _tdc_timestamp4.push_back(ts->Timestamp() + ts->Offset()); 
        _tdc_name4.push_back(ts->Name()); 
        nch4++;  
      }  
      std::cout << " Chan" << ts->Channel() << " " << ts->Name() << " has timestamp " << ts->Timestamp() << " ns and offset " << ts->Offset() << " ns " << std::endl; 
   }

  std::cout << "Event " << _event << " has " << nDAQTimestamps << " timestamps." << std::endl; 
  std::cout << "nCRTT1 = " << nch0 << std::endl; 
  std::cout << "nBES = " << nch1 << std::endl; 
  std::cout << "nRWM = " << nch2 << std::endl; 
  std::cout << "nFTRIG = " << nch3 << std::endl; 
  std::cout << "nETRIG = " << nch4 << std::endl; 
   
  bool ch0exists = (nch0>0); 
  bool ch1exists = (nch1>0); 
  bool ch2exists = (nch2>0); 
  bool ch3exists = (nch3>0); 
  bool ch4exists = (nch4>0); 
 
  //------------------------------------------------------------------------------// 
  // TODO: Do math metrics here 
   
  /* Reminder: SPEC TDC channel inputs 
     ch0: CRT T1 reset (from PTB)  -- once per event 
     ch1: BES (Beam Early Signal) -- once per event 
     ch2: RWM (Resistor Wall Monitor) -- once per event 
     ch3: FTRIG (Flash trigger from PTB)  -- multiple per event 
     ch4: ETRIG (Event trigger from PTB)  -- once per event 
  */ 


  // Metric 1: Exact 1 ETRIG 
  bool oneETRIG = false; 
  if (nch4 == 1) oneETRIG = true; 
  std::cout << "Is there 1 ETRIG? " << oneETRIG << std::endl; 
  if(nch4 != 1){ 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch4 << " ETRIG in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch4 << " ETRIG in it!\n"; 
  }   
 
  // Metric 2: Exact 1 CRT T1 Reset (different number of CRT T1 for different streams) 
  bool oneCRT = false; 
  if (nch0 == 1) oneCRT = true; 
  std::cout << "Is there 1 CRT T1 reset? " << oneCRT << std::endl;   
  if(nch0 != 1){ 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch0 << " CRT T1 reset in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch0 << " CRT T1 resst in it!\n"; 
  } 
 
  // Metric 3: ~10 FTRIG 
  bool manyFTRIG = false; 
  if (nch3 > 20) manyFTRIG = true; 
  std::cout << "Are there many (~10) FTRIGs? " << manyFTRIG  << std::endl; 
  if(nch3 <= 20) std::cout << " Number of FTRIG is "<< nch3 << std::endl; 

  // Metric 4: Exact 1 BES 
  bool oneBES = false; 
  if (nch1 == 1) oneBES = true; 
  std::cout << "Is there 1 BES? " << oneBES << std::endl; 
  if(nch1 == 0) std::cout << " How many BES?  "<< nch1 << std::endl; 
  if (nch1 == 0 and nch2 > 0){ 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has 0 BES, but " << nch2 << " RWM in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has 0 BES, but " << nch2 << " RWM in it!\n"; 
  } 
  if (nch1 > 1 ){ 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES in it!\n";  
  } 
 
  // Metric 5: Exact 1 RWM 
  bool oneRWM = false; 
  if (nch2 == 1) oneRWM = true; 
  std::cout << "Is there 1 RWM? " << oneRWM << std::endl; 
  if(nch2 == 0) std::cout << " How many RWM? "<< nch2 << std::endl; 
  if (nch2 > 1 ){ 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch2 << " RWM in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch2 << " RWM in it!\n"; 
  } 

  // Metric 6: RWM - BES diff constant 
  double RWM_BES_diff = 1e20; 
  bool RWM_BES_const = false;   
  if (!(ch1exists and ch2exists)){ 
    std::cout << "RWM_BES_diff filled with dummy value : 1e20" << std::endl; 
    std::cout << " Channel 1 populated? " << ch1exists << ",   Channel 2 populated? " << ch2exists << std::endl; 
    RWM_BES_const = true; 
  } 
  else if(oneBES == true && oneRWM == true) { 
    RWM_BES_diff = _tdc_timestamp2.back() - _tdc_timestamp1.back();   
    std::cout << "ts ch1 = " << _tdc_timestamp1.back() << std::endl; 
    std::cout << "ts ch2 = " << _tdc_timestamp2.back() << std::endl;  
    std::cout << " difference is " << RWM_BES_diff << std::endl; 
    RWM_BES_const = (abs(GLOB_RWM_BES_diff -  RWM_BES_diff) < 3); 
  } 
  else { 
    RWM_BES_diff = _tdc_timestamp2.back() - _tdc_timestamp1.back(); 
    std::cout << "ts ch1 = " << _tdc_timestamp1.back() << std::endl; 
    std::cout << "ts ch2 = " << _tdc_timestamp2.back() << std::endl; 
    std::cout << " difference between last RWM and last BES is " << RWM_BES_diff << std::endl; 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES and " << nch2 << " RWM in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES and " << nch2 << "RWM in it!\n"; 
    RWM_BES_const = (abs(GLOB_RWM_BES_diff - RWM_BES_diff) < 3); 
  } 
  GLOB_RWM_BES_diff = RWM_BES_diff; 
  std::cout << "RWM - BES constant? " << RWM_BES_const << std::endl;  
  GLOB_RWM_BES_diff_vec.push_back(RWM_BES_diff);   



  // Metric 7: CRT T1 - BES diff constant 
  double CRT_BES_diff = 1e20; 
  bool CRT_BES_const = false; 
  if (!(ch0exists and ch1exists)){ 
    std::cout << "CRT_BES_diff filled with dummy value : 1e20" << std::endl; 
    std::cout << " Channel 0 populated? " << ch0exists << ",   Channel 1 populated? " << ch1exists << std::endl; 
    CRT_BES_const = true; 
  } 
  else if(oneCRT == true && oneBES == true) { 
    CRT_BES_diff = _tdc_timestamp0.back() - _tdc_timestamp1.back(); 
    std::cout << "ts ch0 = " << _tdc_timestamp0.back() << std::endl; 
    std::cout << "ts ch1 = " << _tdc_timestamp1.back() << std::endl; 
    std::cout << " difference is " << CRT_BES_diff << std::endl; 
    CRT_BES_const = (abs(GLOB_CRT_BES_diff - CRT_BES_diff) < 3); 
  } 
  else { 
    CRT_BES_diff = _tdc_timestamp0.back() - _tdc_timestamp1.back(); 
    std::cout << "ts ch0 = " << _tdc_timestamp0.back() << std::endl; 
    std::cout << "ts ch1 = " << _tdc_timestamp1.back() << std::endl; 
    std::cout << " difference between last CRT T1 reset and last BES is " << CRT_BES_diff << std::endl; 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES and " << nch0 << " CRT T1 reset in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES and " << nch0 << " CRT T1 reset in it!\n"; 
    CRT_BES_const = (abs(GLOB_CRT_BES_diff - CRT_BES_diff) < 3); 
  } 
  GLOB_CRT_BES_diff = CRT_BES_diff; 
  std::cout << "CRT T1 reset - BES constant? " << CRT_BES_const << std::endl; 
  GLOB_CRT_BES_diff_vec.push_back(CRT_BES_diff); 

  // Metric 8: ETRIG - BES diff ~1.6us + jitter 
  //make 1.6 hardcoded for now, but put into the fcl file later 
  double ETRIG_BES_jitter = 20;
  double ETRIG_BES_diff = 1e20; 
  bool ETRIG_BES_const = false; 
  if (!(ch4exists and ch1exists)){ 
    std::cout << "ETRIG_BES_diff filled with dummy value : 1e20" << std::endl; 
    std::cout << " Channel 4 populated? " << ch4exists << ",   Channel 1 populated? " << ch1exists << std::endl; 
    ETRIG_BES_const = true; 
  } 
  else if(oneETRIG == true && oneBES == true) { 
    ETRIG_BES_diff = _tdc_timestamp1.back() - _tdc_timestamp4.back(); 
    std::cout << "ts ch1 = " << _tdc_timestamp1.back() << std::endl; 
    std::cout << "ts ch4 = " << _tdc_timestamp4.back() << std::endl; 
    std::cout << " difference is " << ETRIG_BES_diff << std::endl; 
    ETRIG_BES_const =  (abs(ETRIG_BES_diff - 1600) < ETRIG_BES_jitter ); 
  } 
  else { 
    ETRIG_BES_diff = _tdc_timestamp1.back() - _tdc_timestamp4.back(); 
    std::cout << "ts ch1 = " << _tdc_timestamp1.back() << std::endl; 
    std::cout << "ts ch4 = " << _tdc_timestamp4.back() << std::endl; 
    std::cout << " difference between ETRIG and last BES is " << ETRIG_BES_diff << std::endl; 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES and " << nch4 << " ETRIG in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch1 << " BES and " << nch4 << " ETRIG in it!\n"; 
    ETRIG_BES_const = (abs(ETRIG_BES_diff - 1600) < ETRIG_BES_jitter) ; 
  }
  GLOB_ETRIG_BES_diff = ETRIG_BES_diff; 
  std::cout << "ETRIG - BES constant difference of 1.6us + jitter? " << ETRIG_BES_const << std::endl; 
  GLOB_ETRIG_BES_diff_vec.push_back(ETRIG_BES_diff); 



  // Metric 9: ETRIG - RWM diff ~1.6us + jitter
  double ETRIG_RWM_jitter = 20;  
  double ETRIG_RWM_diff = 1e20; 
  bool ETRIG_RWM_const = false; 
  if (!(ch4exists and ch2exists)){ 
    std::cout << "ETRIG_RWM_diff filled with dummy value : 1e20" << std::endl; 
    std::cout << " Channel 4 populated? " << ch4exists << ",   Channel 2 populated? " << ch2exists << std::endl; 
    ETRIG_RWM_const = true; 
  } 
  else if(oneETRIG == true && oneRWM == true) { 
    ETRIG_RWM_diff = _tdc_timestamp4.back() - _tdc_timestamp2.back(); 
    std::cout << "ts ch2 = " << _tdc_timestamp2.back() << std::endl; 
    std::cout << "ts ch4 = " << _tdc_timestamp4.back() << std::endl; 
    std::cout << " difference is " << ETRIG_RWM_diff << std::endl; 
    ETRIG_RWM_const =  (abs(ETRIG_RWM_diff - 1600) < ETRIG_RWM_jitter ); 
  } 
  else { 
    ETRIG_RWM_diff = _tdc_timestamp4.back() - _tdc_timestamp2.back(); 
    std::cout << "ts ch2 = " << _tdc_timestamp2.back() << std::endl; 
    std::cout << "ts ch4 = " << _tdc_timestamp4.back() << std::endl; 
    std::cout << " difference between ETRIG and RWM is " << ETRIG_RWM_diff << std::endl; 
    mf::LogError("sbndaq::SPECTDCStreams::analyze") 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch2 << " RWM and " << nch4 << " ETRIG in it!\n"; 
    std::cout 
         << "Data product '" << m_DAQTimestamp_label << "' has " << nch2 << " RWM and " << nch4 << " ETRIG in it!\n"; 
    ETRIG_RWM_const = (abs(ETRIG_RWM_diff - 1600) < ETRIG_RWM_jitter) ; 
  }
  GLOB_ETRIG_RWM_diff = ETRIG_RWM_diff; 
  std::cout << "ETRIG - RWM constant difference of 1.6us + jitter? " << ETRIG_RWM_const << std::endl; 
  GLOB_ETRIG_RWM_diff_vec.push_back(ETRIG_RWM_diff); 

  // Metric 10: ETRIG - FTRIG diff ~3ms + jitter
  double ETRIG_FTRIG_jitter = 2000;  
  double ETRIG_FTRIG_diff = 1e20; 
  bool ETRIG_FTRIG_const = false; 
  if (!(ch4exists and ch3exists)){ 
    std::cout << "ETRIG_FTRIG_diff filled with dummy value : 1e20" << std::endl; 
    std::cout << " Channel 4 populated? " << ch4exists << ",   Channel 3 populated? " << ch3exists << std::endl; 
    ETRIG_FTRIG_const = true; 
  } 
  else { 
    ETRIG_FTRIG_diff = _tdc_timestamp4.back() - _tdc_timestamp3.back(); 
    std::cout << "ts ch3 = " << _tdc_timestamp3.back() << std::endl; 
    std::cout << "ts ch4 = " << _tdc_timestamp4.back() << std::endl; 
    std::cout << " difference between ETRIG and last FTRIG is " << ETRIG_FTRIG_diff << std::endl; 
    ETRIG_FTRIG_const = (abs(ETRIG_FTRIG_diff - 3000000) < ETRIG_FTRIG_jitter) ; 
  } 
  GLOB_ETRIG_FTRIG_diff = ETRIG_FTRIG_diff;
  std::cout << "ETRIG - FTRIG constant difference of 3ms + jitter? " << ETRIG_FTRIG_const << std::endl; 
  GLOB_ETRIG_FTRIG_diff_vec.push_back(ETRIG_FTRIG_diff); 



  // Metric 11: BES - FTRIG diff ~3ms + jitter
  double BES_FTRIG_jitter = 2000;    
  double BES_FTRIG_diff = 1e20; 
  bool BES_FTRIG_const = false; 
  if (!(ch1exists and ch3exists)){ 
    std::cout << "BES_FTRIG_diff filled with dummy value : 1e20" << std::endl; 
    std::cout << " Channel 1 populated? " << ch1exists << ",   Channel 3 populated? " << ch3exists << std::endl; 
    BES_FTRIG_const = true; 
  } 
  else { 
    BES_FTRIG_diff = _tdc_timestamp1.back() - _tdc_timestamp3.back(); 
    std::cout << "ts ch3 = " << _tdc_timestamp3.back() << std::endl; 
    std::cout << "ts ch1 = " << _tdc_timestamp1.back() << std::endl; 
    std::cout << " difference between BES and last FTRIG is " << BES_FTRIG_diff << std::endl; 
    BES_FTRIG_const = (abs(BES_FTRIG_diff - 3000000) < BES_FTRIG_jitter) ; 
  } 
  GLOB_BES_FTRIG_diff = BES_FTRIG_diff;
  std::cout << "BES - FTRIG constant difference of 3ms + jitter? " << BES_FTRIG_const << std::endl; 
  GLOB_BES_FTRIG_diff_vec.push_back(BES_FTRIG_diff); 


  //Metric 12: Channels numbers agree with Channel names 
  //not sent out in testing stage as people rename things differently in their test data
  bool same_name0 = false; 
  bool same_name1 = false; 
  bool same_name2 = false; 
  bool same_name3 = false;
  bool same_name4 = false; 
   
  std::string str_inp0("crt"); 
  std::string str_inp1("bes"); 
  std::string str_inp2("rwm"); 
  std::string str_inp3("ftrig"); 
  std::string str_inp4("etrig");

  if (ch0exists == true){ 
    for(unsigned i = 0; i < _tdc_name0.size(); ++i) { 
       std::string name0 = _tdc_name0[i]+ ""; 
       bool same_name = strcasecmp(name0.c_str(),str_inp0.c_str());       
      if (same_name == 0) same_name0 = true; 
      else{ 
         same_name0 = false; 
       } 
    } 
    std::cout << "Channel 0 name is correct? " << same_name0 << std::endl; 
  } 
  else std::cout << "Channel 0 is empty" << std::endl; 

  if (ch1exists == true){  
    for(unsigned i = 0; i < _tdc_name1.size(); ++i) { 
       std::string name1 = _tdc_name1[i]+ ""; 
       bool same_name = strcasecmp(name1.c_str(),str_inp1.c_str()); 
       if (same_name == 0) same_name1 = true; 
       else{ 
         same_name1 = false; 
       } 
    } 
    std::cout << "Channel 1 name is correct? " << same_name1 << std::endl; 
  } 
  else std::cout << "Channel 1 is empty" << std::endl; 

  if (ch2exists == true){ 
    for(unsigned i = 0; i < _tdc_name2.size(); ++i) { 
       std::string name2 = _tdc_name2[i]+ ""; 
       bool same_name = strcasecmp(name2.c_str(),str_inp2.c_str()); 
       if (same_name == 0) same_name2 = true; 
       else{ 
         same_name2 = false; 
       } 
    } 
    std::cout << "Channel 2 name is correct? " << same_name2 << std::endl; 
  } 
  else std::cout << "Channel 2 is empty" << std::endl; 
   
  if (ch3exists == true){ 
    for(unsigned i = 0; i < _tdc_name3.size(); ++i) { 
       std::string name3 = _tdc_name3[i]+ ""; 
       bool same_name = strcasecmp(name3.c_str(),str_inp3.c_str()); 
       if (same_name == 0) same_name3 = true; 
       else{ 
         same_name3 = false; 
       } 
    } 
    std::cout << "Channel 3 name is correct? " << same_name3 << std::endl; 
  } 
  else std::cout << "Channel 3 is empty" << std::endl; 

  if(ch4exists == true){ 
    for(unsigned i = 0; i < _tdc_name4.size(); ++i) { 
       std::string name4 = _tdc_name4[i]+ ""; 
       bool same_name = strcasecmp(name4.c_str(),str_inp4.c_str()); 
       if (same_name == 0) same_name4 = true; 
       else{ 
         same_name4 = false; 
       } 
    } 
    std::cout << "Channel 4 name is correct? " << same_name4 << std::endl; 
  } 
  else std::cout << "Channel 4 is empty" << std::endl; 

  //Metric 13: Channels are populated? 
  std::cout << "Channel 0 is populated? " << ch0exists << std::endl; 
  std::cout << "Channel 1 is populated? " << ch1exists << std::endl; 
  std::cout << "Channel 2 is populated? " << ch2exists << std::endl; 
  std::cout << "Channel 3 is populated? " << ch3exists << std::endl; 
  std::cout << "Channel 4 is populated? " << ch4exists << std::endl; 

  //Clearing vectors after run_num = 100 
  run_num++; 
  if (run_num % 100 == 0){ 
    GLOB_RWM_BES_diff_vec.clear(); 
    GLOB_CRT_BES_diff_vec.clear(); 
    GLOB_ETRIG_BES_diff_vec.clear();
    GLOB_ETRIG_RWM_diff_vec.clear();
    GLOB_ETRIG_FTRIG_diff_vec.clear(); 
    GLOB_BES_FTRIG_diff_vec.clear();
  } 


  //------------------------------------------------------------------------------// 
  // TODO: Then send metrics 
  // Example: change metric_ID (i.e. channel ID), metric_name (i.e. channel name), metric_value (i.e. channel timestamp) 
  // sbndaq::sendMetric(groupName, metric_ID, "metric_name", metric_value , level, mode);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "oneETRIG", oneETRIG, 0, artdaq::MetricMode::LastPoint); 
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "oneCRT", oneCRT, 0, artdaq::MetricMode::LastPoint);   
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "manyFTRIG", manyFTRIG, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "oneBES", oneBES, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "oneRWM", oneRWM, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "RWM_BES_const", RWM_BES_const, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "CRT_BES_const", CRT_BES_const, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_BES_diff", ETRIG_BES_diff, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_RWM_diff", ETRIG_RWM_diff, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ETRIG_FTRIG_diff", ETRIG_FTRIG_diff, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "BES_FTRIG_diff", BES_FTRIG_diff, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ch0exists", ch0exists, 0, artdaq::MetricMode::LastPoint);  
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ch1exists", ch1exists, 0, artdaq::MetricMode::LastPoint); 
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ch2exists", ch2exists, 0, artdaq::MetricMode::LastPoint); 
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ch3exists", ch3exists, 0, artdaq::MetricMode::LastPoint); 
  sbndaq::sendMetric("SPECTDC_Streams_Timing", "0", "ch4exists", ch4exists, 0, artdaq::MetricMode::LastPoint);  


  std::cout << "--------------Finish event " << _event << std::endl << std::endl; 
} 
DEFINE_ART_MODULE(sbndaq::SPECTDCStreams) 
