////////////////////////////////////////////////////////////////////////
// 
// TriggerStreams_module.cc 
// 
// Andrea Scarpelli ( ascarpell@bnl.gov )
//
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "canvas/Utilities/Exception.h"

#include "sbndqm/dqmAnalysis/Trigger/detail/KeyedCSVparser.h"
#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"
#include "sbndqm/Decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

#include "lardataobj/RawData/ExternalTrigger.h"
#include "lardataobj/RawData/TriggerData.h"
#include "lardataobj/Simulation/BeamGateInfo.h"

#include "artdaq-core/Data/Fragment.hh"

#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <string_view>

#include "messagefacility/MessageLogger/MessageLogger.h"


namespace sbndaq {

  class TriggerMonitor : public art::EDAnalyzer {

    public:

        explicit TriggerMonitor(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  
        void beginJob() override;
        
        void analyze(art::Event const & evt) override;
        
  
    private:
    
        art::InputTag m_trigger_tag;

        std::string m_redis_hostname;
        int m_redis_port;
        double stringTime = 0.0;

        fhicl::ParameterSet m_metric_config;

        static std::string_view firstLine
            (std::string const&s, std::string const& endl );

        static std::uint64_t makeTimestamp( unsigned int s, unsigned int ns )
            { return s * 1000000000ULL + ns; }
        
        void clean();
    
        std::vector<std::string> bitsToName{ "Offbeam_BNB", "BNB", "Offbeam_NuMI", "NuMI" };
        std::unordered_map<int, int> bitsCountsMap;
 

        struct gt_data_st {
        uint64_t  gt_beamTS;
        uint64_t  gt_globalTriggerTS;
        uint64_t  gt_enableTS;
        uint64_t  gt_gateType;
        uint64_t  gt_triggerType;
        uint64_t  gt_triggerSource;
        uint64_t  gt_cryo1East01;
        uint64_t  gt_cryo1East23;
        uint64_t  gt_cryo2West01;
        uint64_t  gt_cryo2West23;

        }; 
 
        int n_ele = 0;

        std::vector<gt_data_st> gt_info;

        int n_gt_bnb_error  =0;
        int n_gt_numi_error =0;
        std::string const BNB_gate { "BNB" };
        std::string const NuMI_gate { "NuMI" };

        int n_cryostat1_rate =0;
        int n_cryostat2_rate =0;
        std::string rate_cryostat1;
        std::string rate_cryostat2;

        int n_cryostat1_east01_bit = 0;
        int n_cryostat1_east02_bit = 0;
        std::string bits_cryostat1_east01;
        std::string bits_cryostat1_east02;

        //vector<int> cryo1_east01_bits(64);
        //vector<int> cryo1_east02_bits(64);

        int n_BNB_Majority_rate = 0;
        int n_BNB_Minbias_rate  = 0;
        int n_BNB_Offbeam_Majority_rate = 0;
        int n_BNB_Offbeam_Minbias_rate  = 0;
        std::string rate_BNB_Majority;
        std::string rate_BNB_Minbias;
        std::string rate_BNB_Offbeam_Majority;
        std::string rate_BNB_Offbeam_Minbias; 

        int n_NUMI_Majority_rate = 0;
        int n_NUMI_Minbias_rate  = 0;
        int n_NUMI_Offbeam_Majority_rate = 0;
        int n_NUMI_Offbeam_Minbias_rate  = 0;
        std::string rate_NUMI_Majority;
        std::string rate_NUMI_Minbias;
        std::string rate_NUMI_Offbeam_Majority;
        std::string rate_NUMI_Offbeam_Minbias;

        int n_GT_BNB_error = 0;
        int n_GT_NUMI_error = 0;
        int n_BNB_enable_error = 0;
        int n_NUMI_enable_error = 0;
        std::string GT_BNB_error;;
        std::string GT_NUMI_error;
        std::string BNB_enable_error;
        std::string NUMI_enable_error;

        uint64_t cryo1East01 = 0;
        uint64_t cryo1East02 = 0;

        enum GateIndex_t { ixBNB, ixNuMI, NGateIndices };
        
        
        /// Data for a gate type (BNB, NuMI etc.)
        struct GateCounters_t {
            
            /// Number of events displayed for spill time plots
            /// (same number for all gates; one channel per event)
            static constexpr std::size_t NSpillTimeChannels = 500;
            
            /// Highest accepted value for the spill time [ns]
            double SpillTimeMaxAlarm;
            
            /// Number of spill times above the accepted interval.
            unsigned int nSpillTimeMaxAlarms = 0;
            
            std::string name; ///< Name of the gate.
            
            /// The next available channel for spill time plots.
            std::size_t nextSpillTimeChannel = 0;
            
        }; // GateCounters_t
        
        /// Gate counting information for all gates (index as in `GateIndex_t`).
        std::array<GateCounters_t, NGateIndices> fGateCounters;
        
        
        // --- BEGIN -- settings to match to configuration ---------------------
        
        /// Group for general trigger metrics.
        static std::string const groupName;
        
        /// Group for gate-specific metrics.
        static std::string const gateGroupName;
        
        static std::string const instanceName;
        
        static constexpr int level = 3;
        
        // --- END ---- settings to match to configuration ---------------------

        void ResetMetricChannels();
        
        /// Returns the channel name for the next event of the specified gate.
        /// It increments the counters accordingly.
        std::string nextSpillTimeChannel(GateIndex_t gateIndex)
            {
                GateCounters_t& gateCounters = fGateCounters.at(gateIndex);
                return std::to_string(
                    gateCounters.nextSpillTimeChannel++
                    % gateCounters.NSpillTimeChannels
                );
            }

 };

}

std::string const sbndaq::TriggerMonitor::groupName = "Trigger";
std::string const sbndaq::TriggerMonitor::gateGroupName = "TriggerGate";

std::string const sbndaq::TriggerMonitor::instanceName{ "0" };
//std::string const sbndaq::TriggerMonitor::instanceName1{ "" };


//------------------------------------------------------------------------------------------------------------------
sbndaq::TriggerMonitor::TriggerMonitor(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , m_trigger_tag{ pset.get<art::InputTag>("TriggerLabel") }
  , m_redis_hostname{ pset.get<std::string>("RedisHostname", "icarus-db01") }
  , m_redis_port{ pset.get<int>("RedisPort", 6379) }
{

  // Configure the redis metrics
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));
  
  /// initialization of the counters
  GateCounters_t* gateCounters = nullptr;
  
  gateCounters = &(fGateCounters[ixBNB]);
  gateCounters->name              = "BNB";
  gateCounters->SpillTimeMaxAlarm = 1600.0; // ns
  
  gateCounters = &(fGateCounters[ixNuMI]);
  gateCounters->name              = "NuMI";
  gateCounters->SpillTimeMaxAlarm = 9600.0; // ns
  
}

//------------------------------------------------------------------------------------------------------------------


std::string_view sbndaq::TriggerMonitor::firstLine
 ( std::string const&s, std::string const& endl )
{
	return{ s.data(), std::min(s.find_first_of(endl), s.size()) };	
}
 


void sbndaq::TriggerMonitor::clean() {
  
  bitsCountsMap.clear();

}



//------------------------------------------------------------------------------------------------------------------
void sbndaq::TriggerMonitor::beginJob() {

  ResetMetricChannels();

}

//------------------------------------------------------------------------------------------------------------------
void sbndaq::TriggerMonitor::ResetMetricChannels() {
  // we send to all channels a value out of the range of the variable,
  // so that they don't clog the plots while they haven't been assigned a real value yet
  for (GateCounters_t const& counters: fGateCounters) {
    std::string const metricName = counters.name + "spillTime";
    
    for (std::size_t channel = 0; channel < counters.NSpillTimeChannels; ++channel) {
      std::string const channelName = std::to_string(channel);
      sbndaq::sendMetric
        (gateGroupName, channelName, metricName, -999.0, level, artdaq::MetricMode::LastPoint);
    } // for channels
  } // for gates
} // sbndaq::TriggerMonitor::ResetMetricChannels()


//------------------------------------------------------------------------------------------------------------------


void sbndaq::TriggerMonitor::analyze(art::Event const & evt) {

  // std::vector<gt_data_st> gt_info;
  // example of a GT trigger string sent via TCP/IP
  /*
 string received::  Version, 1  , Local_TS1 , 85644 , 017252 , 800916704 , WR_TS1 , 85644 , 01657831702 , 595326144 , Enable Type , 5 , Enable_TS , 0  , 01657831702 , 594321976 , Gate ID , 85644 , BNB Beam Gate ID , 0 , NuMI Beam Gate ID , 0 , Offbeam BNB Gate ID , 0 , Offbeam NuMI Gate ID , 0 , Gate Type , 5 , Beam_TS , 85644 , 01657831702 , 595321976 , Trigger Type , 1 , Trigger Source , 0 , Cryo1 EAST Connector 0 and 1, 500000000586A , Cryo1 EAST Connector 2 and 3, 4 , Cryo2 WEST Connector 0 and 1, 800000000 , Cryo2 WEST Connector 2 and 3, 200 , Cryo1 EAST counts, 0 , Cryo2 WEST counts, 0^M
  */

  
  //Redis stuff
  artdaq::MetricMode const rate = artdaq::MetricMode::Rate;
  artdaq::MetricMode const mode = artdaq::MetricMode::Average;
  artdaq::MetricMode const value = artdaq::MetricMode::LastPoint;

  auto const & daqHandle = evt.getValidHandle<artdaq::Fragments>(m_trigger_tag);
  for( auto const & rawFrag: *daqHandle ){
	
	std::string data((char*)rawFrag.dataBeginBytes(), 1000);
        std::cout << " data dump" << data << std::endl;

        std::cout << " first line dump" << firstLine(data, "\n") << std::endl;

        n_ele ++;

        //Beam_TS	
	uint64_t beamGate = 0;
	auto const parseData = icarus::details::KeyedCSVparser{}(firstLine(data, "\n"));
	if(auto pBeamGateInfo = parseData.findItem("Beam_TS");
		pBeamGateInfo && ( pBeamGateInfo->nValues() ==3 )
	){
		uint64_t const rawBeamGateTS = makeTimestamp(
			pBeamGateInfo->getNumber<unsigned int>(1U),
			pBeamGateInfo->getNumber<unsigned int>(2U)
		);

		beamGate = rawBeamGateTS - 1'000'000'000ULL; 
	}

	std::cout << "-------------> Beam_TS          : " << beamGate << std::endl;


        uint64_t globalTriggerTS = 0;
        //auto const parseData1 = icarus::details::KeyedCSVparser{}(firstLine(data, "\r"));
        if(auto pGlobalTriggerInfo = parseData.findItem("WR_TS1");
                pGlobalTriggerInfo && ( pGlobalTriggerInfo->nValues() ==3 )
        ){
                uint64_t const rawGlobalTriggerTS = makeTimestamp(
                        pGlobalTriggerInfo->getNumber<unsigned int>(1U),
                        pGlobalTriggerInfo->getNumber<unsigned int>(2U)
                );

                globalTriggerTS = rawGlobalTriggerTS - 1'000'000'000ULL;
        }

        std::cout << "-------------> GlobaTrigger_TS  : " << globalTriggerTS << std::endl;


        //Enable_TS  

        uint64_t enableTS = 0;
        //auto const parseData2 = icarus::details::KeyedCSVparser{}(firstLine(data, "\r"));
        if(auto pEnableInfo = parseData.findItem("Enable_TS");
                pEnableInfo && ( pEnableInfo->nValues() ==3 )
        ){
                uint64_t const rawEnableTS = makeTimestamp(
                        pEnableInfo->getNumber<unsigned int>(1U),
                        pEnableInfo->getNumber<unsigned int>(2U)
                );

                enableTS = rawEnableTS - 1'000'000'000ULL;
        }

        std::cout << "-------------> Enable_TS        : " << enableTS << std::endl;



        //Gate_Type

        uint64_t gateType = 0;
        //auto const parseData3 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));
        if(auto pGateTypeInfo = parseData.findItem("Gate Type");
                pGateTypeInfo && ( pGateTypeInfo->nValues() == 1 )
        ){
                uint64_t const rawGateType = pGateTypeInfo->getNumber<unsigned int>(0U);

                gateType = rawGateType ;
        }

        std::cout << "-------------> Gate Type        : " << gateType << std::endl;


        //Enable_Type

        uint64_t enableType = 0;
        //auto const parseData4 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));
        if(auto pEnableTypeInfo = parseData.findItem("Enable Type");
                pEnableTypeInfo && ( pEnableTypeInfo->nValues() == 1 )
        ){
                uint64_t const rawEnableType = pEnableTypeInfo->getNumber<unsigned int>(0U);

                enableType = rawEnableType ;
        }

        std::cout << "-------------> Enable Type      : " << enableType << std::endl;



        //Trigger_Type
       
        uint64_t triggerType = 0;
        //auto const parseData5 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));
        if(auto pTriggerTypeInfo = parseData.findItem("Trigger Type");
                pTriggerTypeInfo && ( pTriggerTypeInfo->nValues() == 1 )
        ){
                uint64_t const rawTriggerType = pTriggerTypeInfo->getNumber<unsigned int>(0U);

                triggerType = rawTriggerType ;
        }

        std::cout << "-------------> Trigger Type     : " << triggerType << std::endl;


       //Trigger_Source

        uint64_t triggerSource = 0;
        //auto const parseData6 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));
        if(auto pTriggerSourceInfo = parseData.findItem("Trigger Source");
                pTriggerSourceInfo && ( pTriggerSourceInfo->nValues() == 1 )
        ){
                uint64_t const rawTriggerSource = pTriggerSourceInfo->getNumber<unsigned int>(0U);

                triggerSource = rawTriggerSource ;
        }

        std::cout << "-------------> Trigger Source   : " << triggerSource << std::endl;


       //Cryo1_East_0_1

        //uint64_t cryo1East01 = 0;
        cryo1East01 = 0;
        //std::stringstream ss_cryo1;
        // auto const parseData7 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));
        icarus::details::KeyedCSVparser parser;
        parser.addPatterns({
        { "Cryo. (EAST|WEST) Connector . and .", 1U }
         , { "Trigger Type", 1U }
        });
        //auto const parseData = parser(firstLine(data, "\0\n\r"));
        if(auto pCryo1East01Info = parseData.findItem("Cryo1 EAST Connector 0 and 1");
                pCryo1East01Info && ( pCryo1East01Info->nValues() == 1 )
        ){
                
                //this converts to  integer
                uint64_t const rawCryo1East01Info = pCryo1East01Info->getNumber<std::uint64_t>(0,16);

                cryo1East01 = rawCryo1East01Info ;
                
                //read as hex (and we convert it later)  ??
                //ss_cryo1 = pCryo1East01Info->getNumber<std::hex>(0U,16);
 
        }

        std::cout << "-------------> Cryo1 East 0-1   : " << std::hex << cryo1East01 << std::dec  << std::endl;
        //std::cout << "-------------> Cryo1 East 0-1   : " << ss_cryo1  << std::endl;


       //Cryo1_East_2_3

        uint64_t cryo1East23 = 0;
        //std::stringstream ss_cryo1;
        //auto const parseData8 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));

        //icarus::details::KeyedCSVparser parser;
        parser.addPatterns({
        { "Cryo. (EAST|WEST) Connector . and .", 1U }
         , { "Trigger Type", 1U }
        });
        //auto const parseData8 = parser(firstLine(data, "\0\n\r"));
        if(auto pCryo1East23Info = parseData.findItem("Cryo1 EAST Connector 2 and 3");
                pCryo1East23Info && ( pCryo1East23Info->nValues() == 1 )
        ){

                //this converts to  integer
                uint64_t const rawCryo1East23Info = pCryo1East23Info->getNumber<std::uint64_t>(0,16);

                cryo1East23 = rawCryo1East23Info ;

                //read as hex (and we convert it later)  ??
                //ss_cryo1 = pCryo1East01Info->getNumber<std::hex>(0U,16);

        }

        std::cout << "-------------> Cryo1 East 2-3   : " << std::hex << cryo1East23 << std::dec  << std::endl;
        //std::cout << "-------------> Cryo1 East 0-1   : " << ss_cryo1  << std::endl;




       //Cryo2_West_0_1

        uint64_t cryo2West01 = 0;
        //std::stringstream ss_cryo1;
        //auto const parseData9 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));

        //icarus::details::KeyedCSVparser parser;
        parser.addPatterns({
        { "Cryo. (EAST|WEST) Connector . and .", 1U }
         , { "Trigger Type", 1U }
        });
        //auto const parseData9 = parser(firstLine(data, "\0\n\r"));
         if(auto pCryo2West01Info = parseData.findItem("Cryo2 WEST Connector 0 and 1");
                pCryo2West01Info && ( pCryo2West01Info->nValues() == 1 )
        ){

                //this converts to  integer
                uint64_t const rawCryo2West01Info = pCryo2West01Info->getNumber<std::uint64_t>(0,16);

                cryo2West01 = rawCryo2West01Info ;

                //read as hex (and we convert it later)  ??
                //ss_cryo1 = pCryo1East01Info->getNumber<std::hex>(0U,16);

        }

        std::cout << "-------------> Cryo2 West 0-1   : " << std::hex << cryo2West01 << std::dec  << std::endl;
        //std::cout << "-------------> Cryo1 East 0-1   : " << ss_cryo1  << std::endl;


       //Cryo2_West_2_3

        uint64_t cryo2West23 = 0;
        //std::stringstream ss_cryo1;
        //auto const parseData10 = icarus::details::KeyedCSVparser{}(firstLine(data, "\0\n\r"));

        //icarus::details::KeyedCSVparser parser;
        parser.addPatterns({
        { "Cryo. (EAST|WEST) Connector . and .", 1U }
         , { "Trigger Type", 1U }
        });
        auto const parseData10 = parser(firstLine(data, "\0\n\r"));
 
        if(auto pCryo2West23Info = parseData.findItem("Cryo2 WEST Connector 2 and 3");
                pCryo2West23Info && ( pCryo2West23Info->nValues() == 1 )
        ){

                //this converts to  integer
                uint64_t const rawCryo2West23Info = pCryo2West23Info->getNumber<std::uint64_t>(0,16);

                cryo2West23 = rawCryo2West23Info ;

                //read as hex (and we convert it later)  ??
                //ss_cryo1 = pCryo1East01Info->getNumber<std::hex>(0U,16);
  
        }
  
        std::cout << "-------------> Cryo2 West 2-3   : " << std::hex << cryo2West23 << std::dec  << std::endl;
        //std::cout << "-------------> Cryo1 East 0-1   : " << ss_cryo1  << std::endl;
    

        std::cout << "filling the structure gt_data_st  "  << n_ele << std::endl;

	// Fill the info into the structure
	gt_data_st this_gt{ beamGate, globalTriggerTS, enableTS, gateType, triggerType, triggerSource, cryo1East01, cryo1East23, cryo2West01, cryo2West23  };

	gt_info.push_back( this_gt );

/////////////////}



//now lets use the data saved in the structure

   //how many trigers we had written
   
   std::cout << "size of the structure gt_info  " << gt_info.size() << std::endl;

   int len = (int) gt_info.size();
   std::cout << "Structure len = " << len  << std::endl; 


/////////   for ( int i = 0; i < len  ; i++ ) {

    int i = len-1;
    std::cout << "In structure loop i = " << i << std::endl; 
    //gateType
    std::cout << i << "  " << gt_info[i].gt_gateType << std::endl;
    //beamTS
    std::cout << i << "  " << gt_info[i].gt_beamTS << std::endl;
    //global Trigger TS
    std::cout << i << "  " << gt_info[i].gt_globalTriggerTS << std::endl;

    //check if GT outside beam gate

    double const trigFromBeam
      = ((gt_info[i].gt_globalTriggerTS - gt_info[i].gt_beamTS) - 4000) / 1000.0; // 4000 ns: veto

    // identify which gate we are dealing with:
    GateIndex_t gateIndex = NGateIndices;
    switch (gt_info[i].gt_gateType) {
      case 1: gateIndex = ixBNB; break;
      case 2: gateIndex = ixNuMI; break;
    }
    
    // update gate-dependent metrics (e.g. XXXspillTime)
    if (gateIndex != NGateIndices) { // metrics here only support BNB and NuMI
    
      GateCounters_t& gateCounters = fGateCounters.at(gateIndex);
      
      std::cout 
        << "beam " << gateCounters.name << " (" << gt_info[i].gt_gateType << ")"
        << "\nCheck " << gateCounters.name << " (GT TS - beam TS) :  " << trigFromBeam
        << "\nCall sendMetric spillTime"
        << std::endl;
      
      if ( trigFromBeam > gateCounters.SpillTimeMaxAlarm) {
        std::cout << "ERROR GT TS AFTER end of " << gateCounters.name << " Beam gate !!!" << std::endl;
        ++gateCounters.nSpillTimeMaxAlarms;
      }
      
      std::string metricName;
      metricName = gateCounters.name + "spillTime";
      sbndaq::sendMetric(gateGroupName, nextSpillTimeChannel(gateIndex), metricName, trigFromBeam, level, value);
      
    } // if supported gate
    //

    int doEnable = 0;
    if (doEnable == 1) {
    // check that Beam TS and Enable TS < 1 ms (1000000 ns)
    //if BNB
    if ( gt_info[i].gt_gateType == 1 ) {
       std::cout << "BNB beam " << gt_info[i].gt_gateType << std::endl;
       if ( gt_info[i].gt_beamTS - gt_info[i].gt_enableTS > 1000000 ) {
       std::cout << "ERROR !! Beam gate too far from Enable gate !!! " << std::endl;
       n_BNB_enable_error++;
       std::cout << "Call sendMetric BNB_enable_error " << n_BNB_enable_error << std::endl;
       sbndaq::sendMetric(groupName, BNB_enable_error, " BNB Enable gate error  ", n_BNB_enable_error , level, mode);
       } 
    }

    //if nuMI
    if ( gt_info[i].gt_gateType == 2 ) {
      std::cout << "NuMI beam " << gt_info[i].gt_gateType << std::endl;
      if ( gt_info[i].gt_beamTS - gt_info[i].gt_enableTS > 1000000 ) {
       std::cout << "ERROR !! Beam gate too far to Enable  !!! " << std::endl;
       n_NUMI_enable_error++;
       std::cout << "Call sendMetric NUMI_enable_eror " << n_NUMI_enable_error << std::endl;
       sbndaq::sendMetric(groupName, NUMI_enable_error, "NUMI Enable error  ", n_NUMI_enable_error , level, mode);
      }  
    }

    }//if do Enable

    int  doCryos = 1;
    if (doCryos == 1)  {

    //check on Trigger Source distribution (ie triggers by Cryostat)
    // Cryostat 1
    if ( gt_info[i].gt_triggerSource == 1 ) {
    std::cout << "Trigger source in Cryo1  " << gt_info[i].gt_triggerSource << std::endl;
    //getting the LVDS bits for cryo1East01
    std::cout << "Checking  the bits from cryo1East01   "  << std::hex << cryo1East01 << "   " << std::dec << cryo1East01 << std::endl;     
    std::cout << "Checking  the bits from cryo1East23   "  << std::hex << cryo1East23 << "   " << std::dec << cryo1East23 << std::endl;

    std::cout << "Extracting  the bits from cryo1East01   "  << std::hex << cryo1East01 << "   " << std::dec << cryo1East01 << std::endl;
    std::bitset<64U> LVDSbitsEast01{ cryo1East01 };
    for (int bit = 0; bit < 64; ++bit) {
    //std::cout << "In bits loop  "  << bit << std::endl;
    //????  
    //if (LVDSbitsEast01[bit]) fHistLVDSbitsEast01->Fill(bit);
     if (LVDSbitsEast01[bit]) { 
       std::cout << " bit " << std::dec << bit << std::endl;
       
     //fill vector ??
     //  int pos = cryo1_east01_bits.begin() + bit;
     //  cryo1_east01_bits.fill_n(pos, 1, 1);
     //sending the metric here only sends the non-zero bits
     sbndaq::sendMetric("TriggerLVDS", std::to_string(bit), "LVDSRate_cryo1_east01", 1, level, value);
     }
    //sending the metric here alos send the zeroes
    //sbndaq::sendMetric("TriggerLVDS", std::to_string(bit), "LVDSRate_cryo1_east01", int(LVDSbitsEast01[bit]), level, value);
    }
    //bndaq::sendMetric(gateGroupName, nextSpillTimeChannel(gateIndex), metricName, trigFromBeam, level, value);
    //std::cout << " Vector " << cryo1_east01_bits;
    //send bits
    ///sbndaq::sendMetric(groupName, bits_cryostat1, "Cryostat 1 LVDS bits ", n_cryostat1_bit , level, value);
    
    //getting the LVDS bits for cryo1East02
    std::cout << "Extracting  the bits from cryo1East23   "  << std::hex << cryo1East23 << "   " << std::dec << cryo1East23 << std::endl;
    std::bitset<64U> LVDSbitsEast02{ cryo1East23 };
    for (int bit = 0; bit < 64; ++bit) {
    //std::cout << "In bits loop  "  << bit << std::endl;
    //????  
    //if (LVDSbitsEast01[bit]) fHistLVDSbitsEast01->Fill(bit);
     if (LVDSbitsEast02[bit]) {
       std::cout << " bit " << std::dec << bit << std::endl;
     //fill vector ??
     //sending the metric here only send the non-zero bits
     //sbndaq::sendMetric("TriggerLVDS", std::to_string(bit), "LVDSRate_cryo1_east02", int(LVDSbitsEast02(bit]), level, value);
    }
    //sending the metric here also sends the zeros
    //sbndaq::sendMetric("TriggerLVDS", std::to_string(bit), "LVDSRate_cryo1_east02", int(LVDSbitsEast02[bit]), level, value);
    }

    }//triggersource ==1

    //
    // Cryostat 2
    if ( gt_info[i].gt_triggerSource == 2 ) {
    std::cout << "Trigger source in Cryo1  " << gt_info[i].gt_triggerSource << std::endl;
    n_cryostat2_rate++;
    //send rate;
    //sbndaq::sendMetric(groupName, rate_cryostat2, "Cryostat 2 rate ", n_cryostat2_rate , level, rate);
    }

    }//doCryos


    //check rates by beam type and trigger type
    //BNB beam and offbeam, Majority and Minbias
  
    int  doThis = 0;
    if (doThis == 1) {

    //if BNB beam and Majority
    if ( gt_info[i].gt_gateType == 1 && gt_info[i].gt_triggerType == 0 ) {
    std::cout << "BNB beam Majority" << std::endl;
    n_BNB_Majority_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_BNB_Majority, "BNB Majority rate ", n_BNB_Majority_rate , level, rate);
    }
    //if BNB beam and Minbias
    if ( gt_info[i].gt_gateType == 1 && gt_info[i].gt_triggerType == 1 ) {
    std::cout << "BNB Minbias " << std::endl;
    n_BNB_Minbias_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_BNB_Minbias, "BNB Minbias rate ", n_BNB_Minbias_rate , level, rate);
    }
    //Offbem
    //if BNB OFFFBEAM Majority
    if ( gt_info[i].gt_gateType == 3 && gt_info[i].gt_triggerType == 0 ) {
    std::cout << "BNB Offbeam  Majority" << std::endl;
    n_BNB_Offbeam_Majority_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_BNB_Offbeam_Majority, "BNB Offbeam Majority rate ", n_BNB_Offbeam_Majority_rate , level, rate);
    }
    //if BNB OFFBEAM Minbias
    if ( gt_info[i].gt_gateType == 3 && gt_info[i].gt_triggerType == 1 ) {
    std::cout << "BNB Offbeam Minbias " << std::endl;
    n_BNB_Offbeam_Minbias_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_BNB_Offbeam_Minbias, "BNB Offbeam Minbias rate ", n_BNB_Offbeam_Minbias_rate , level, rate);
    }



    //NuMI beam and offbeam , MAjority and Minbias
    //if NUMI beam and Majority
    if ( gt_info[i].gt_gateType == 2 && gt_info[i].gt_triggerType == 0 ) {
    std::cout << "NUMI beam Majority" << std::endl;
    n_NUMI_Majority_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_NUMI_Majority, "NUMI Majority rate ", n_NUMI_Majority_rate , level, rate);
    }
    //if NUMI beam and Minbias
    if ( gt_info[i].gt_gateType == 2 && gt_info[i].gt_triggerType == 1 ) {
    std::cout << "NUMI Minbias " << std::endl;
    n_NUMI_Minbias_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_NUMI_Minbias, "NUMI Minbias rate ", n_NUMI_Minbias_rate , level, rate);
    }
    //Offbeam
    //if NUMI OFFFBEAM Majority
    if ( gt_info[i].gt_gateType == 4 && gt_info[i].gt_triggerType == 0 ) {
    std::cout << "NUMI Offbeam  Majority" << std::endl;
    n_NUMI_Offbeam_Majority_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_NUMI_Offbeam_Majority, "NUMI Offbeam Majority rate ", n_NUMI_Offbeam_Majority_rate , level, rate);
    }
    //if NUMI OFFBEAM Minbias
    if ( gt_info[i].gt_gateType == 4 && gt_info[i].gt_triggerType == 1 ) {
    std::cout << "NUMI Offbeam Minbias " << std::endl;
    n_NUMI_Offbeam_Minbias_rate++;
    //send rate;
    sbndaq::sendMetric(groupName, rate_NUMI_Offbeam_Minbias, "NUMI Offbeam Minbias rate ", n_NUMI_Offbeam_Minbias_rate , level, rate);
    }

    //Calibration

   } // if doTHis
   //


    //check if any of LVDS are FFFFFF all the time ??F


///////////   }//end for on data structure

}//end of loop on events ??        


  // Now we get the trigger information
  art::Handle< std::vector<raw::Trigger> > triggerHandle;
  evt.getByLabel( m_trigger_tag, triggerHandle );

  if( triggerHandle.isValid() && !triggerHandle->empty() ) {
    for( auto const trigger : *triggerHandle ){
      bitsCountsMap[ trigger.TriggerBits() ]++;
    }
  }   
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product raw::Trigger not found!\n";
  }

  // Here we read the external trigger information
  art::Handle< std::vector<raw::ExternalTrigger> > extTriggerHandle;
  evt.getByLabel( m_trigger_tag, extTriggerHandle );

  if( extTriggerHandle.isValid() && !extTriggerHandle->empty() ) {
    //std::cout << "OK" << std::endl;
  }
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product raw::ExternalTrigger not found!\n";
  }
  
  // Here we read the beam information
  art::Handle< std::vector<sim::BeamGateInfo> > gateHandle;
  evt.getByLabel( m_trigger_tag, gateHandle );

  if( gateHandle.isValid() && !gateHandle->empty() ) {
    //std::cout << "OK" << std::endl;
  }
  else {
    mf::LogError("sbndaq::TriggerStreams::analyze") << "Data product sim::GeteBeamInfo not found!\n";
  }

  // Now we send the metrics. There is probably one trigger per event, but agnostically we created a map to count the different types, hence we send them separarately
  for( const auto bits : bitsCountsMap ){
    
    std::string triggerid_s;

    if( (size_t)bits.first-1 < bitsToName.size() ){
      triggerid_s = bitsToName[ bits.first-1 ];
    }
    else{ triggerid_s = "Unknown"; } 
    
    triggerid_s+="_RATE";

    //Send the trigger rate 
    sbndaq::sendMetric(groupName, triggerid_s, "trigger_rate", bits.second, level, rate);
  
  }
  

  // Sweep the dust away 
  clean();

}

DEFINE_ART_MODULE(sbndaq::TriggerMonitor)
