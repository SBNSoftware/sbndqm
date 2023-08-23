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
        // std::string m_redis_hostname;
        // int m_redis_port;
        // fhicl::ParameterSet m_metric_config;
        
        //Label
        std::string m_DAQTimestamp_label;

        //Define your variables         
        int _event;

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
  // , m_redis_hostname{ pset.get<std::string>("RedisHostname", "sbnd-db") }
  // , m_redis_port{ pset.get<int>("RedisPort", 6379) }
  // , m_metric_config{ pset.get<fhicl::ParameterSet>("SPECTDCMetricConfig") }
  , m_DAQTimestamp_label{ pset.get<std::string>("DAQTimestampLabel") }
{
}

void sbndaq::SPECTDCStreams::analyze(art::Event const & e) {

  //Define metric for Redis (TODO: UnComment) 
  //int level = 3; 
  //artdaq::MetricMode mode = artdaq::MetricMode::Average;
  //artdaq::MetricMode rate = artdaq::MetricMode::Rate;
  //std::string groupName = "SPECTDC";

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
 
  for(unsigned i = 0; i < nDAQTimestamps; ++i) {
    auto ts = DAQTimestampVec[i];
    
    if (ts->Channel() == 0) nch0++;
    if (ts->Channel() == 1) nch1++;
    if (ts->Channel() == 2) nch2++;
    if (ts->Channel() == 3) nch3++;
    if (ts->Channel() == 4) nch4++;

    std::cout << " Chan" << ts->Channel() << " " << ts->Name() << " has timestamp " << ts->Timestamp() << " ns and offset " << ts->Offset() << " ns " << std::endl;
  }
  
  std::cout << "Event " << _event << " has " << nDAQTimestamps << " timestamps." << std::endl;
  std::cout << "nCRTT1 = " << nch0 << std::endl;
  std::cout << "nBES = " << nch1 << std::endl;
  std::cout << "nRWM = " << nch2 << std::endl;
  std::cout << "nFTRIG = " << nch3 << std::endl;
  std::cout << "nETRIG = " << nch4 << std::endl;
  
  for(unsigned i = 0; i < nDAQTimestamps; ++i) {
      auto ts = DAQTimestampVec[i];
      
      if (ts->Channel() == 0) {
        _tdc_timestamp0.push_back(ts->Timestamp() + ts->Offset());
        _tdc_name0.push_back(ts->Name());
      } 
      
      if (ts->Channel() == 1) {
        _tdc_timestamp1.push_back(ts->Timestamp() + ts->Offset());
        _tdc_name1.push_back(ts->Name());
      } 
      
      if (ts->Channel() == 2) {
        _tdc_timestamp2.push_back(ts->Timestamp() + ts->Offset());
        _tdc_name2.push_back(ts->Name());
      } 
      
      if (ts->Channel() == 3) {
        _tdc_timestamp3.push_back(ts->Timestamp() + ts->Offset());
        _tdc_name3.push_back(ts->Name());
      } 
      
      if (ts->Channel() == 4) {
        _tdc_timestamp4.push_back(ts->Timestamp() + ts->Offset());
        _tdc_name4.push_back(ts->Name());
      } 
   }
 
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

  // Metric 2: Exact 1 CRT T1 Reset (different number of CRT T1 for different streams)

  // Metric 3: ~10 FTRIG

  // Metric 4: Exact 1 BES
  bool oneBES = false;
  if (nch1 == 1) oneBES = true; 
  std::cout << "Is there 1 BES? " << oneBES << std::endl;  

  // Metric 5: Exact 1 RWM
  bool oneRWM = false;
  if (nch0 == 1) oneRWM = true; 
  std::cout << "Is there 1 RWM? " << oneRWM << std::endl;  

  // Metric 6: RWM - BES diff constant
  //If you get exact one RWM and BES
  if( oneBES == true && oneRWM == true) {
    //Do something here
   
  }
  //What if you don't get exact one?
  // else {}

  // Metric 7: CRT T1 - BES diff constant

  // Metric 8: ETRIG - BES diff ~1.6us + jitter

  // Metric 9: ETRIG - RWM diff ~1.6us + jitter

  // Metric 10: ETRIG - FTRIG diff ~3ms + jitter

  // Metric 11: BES - FTRIG diff ~3ms + jitter  

  //------------------------------------------------------------------------------//
  // TODO: Then send metrics
  // Example: change metric_ID (i.e. channel ID), metric_name (i.e. channel name), metric_value (i.e. channel timestamp)
  // sbndaq::sendMetric(groupName, metric_ID, "metric_name", metric_value , level, mode); 

  std::cout << "--------------Finish event " << _event << std::endl << std::endl;
}
DEFINE_ART_MODULE(sbndaq::SPECTDCStreams)
