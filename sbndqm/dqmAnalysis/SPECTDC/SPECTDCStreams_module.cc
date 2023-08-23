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

        std::vector<uint32_t>    _tdc_channel;
        std::vector<uint64_t>    _tdc_timestamp;
        std::vector<uint64_t>    _tdc_offset;
        std::vector<std::string> _tdc_name;
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
  unsigned nCRTT1 = 0;
  unsigned nBES = 0;
  unsigned nRWM = 0;
  unsigned nFTRIG = 0;
  unsigned nETRIG = 0;
 
  _tdc_channel.resize(nDAQTimestamps);
  _tdc_timestamp.resize(nDAQTimestamps);
  _tdc_offset.resize(nDAQTimestamps);
  _tdc_name.resize(nDAQTimestamps);
 
  for(unsigned i = 0; i < nDAQTimestamps; ++i) {
      auto ts = DAQTimestampVec[i];
      
      if (ts->Channel() == 0) nCRTT1++;
      if (ts->Channel() == 1) nBES++;
      if (ts->Channel() == 2) nRWM++;
      if (ts->Channel() == 3) nFTRIG++;
      if (ts->Channel() == 4) nETRIG++;
     
      _tdc_channel[i] = ts->Channel();
      _tdc_name[i] = ts->Name();
      _tdc_timestamp[i] = ts->Timestamp();
      _tdc_offset[i] = ts->Offset();

      std::cout << " Chan" << ts->Channel() << " " << ts->Name() << " has timestamp " << ts->Timestamp() << " ns and offset " << ts->Offset() << " ns " << std::endl;
  }
  
  std::cout << "Event " << _event << " has " << nDAQTimestamps << " timestamps." << std::endl;
  std::cout << "nCRTT1 = " << nCRTT1 << std::endl;
  std::cout << "nBES = " << nBES << std::endl;
  std::cout << "nRWM = " << nRWM << std::endl;
  std::cout << "nFTRIG = " << nFTRIG << std::endl;
  std::cout << "nETRIG = " << nETRIG << std::endl;
 
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

  // Metric 5: Exact 1 RWM

  // Metric 6: RWM - BES diff constant

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
