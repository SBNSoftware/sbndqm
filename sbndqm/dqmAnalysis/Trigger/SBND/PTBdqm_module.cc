////////////////////////////////////////////////////////////////////////
// 
// PTBdqm_module.cc 
// 
// Beth Slater ( b.slater2@liverpool.ac.uk )
// Gabriela Vitti Stenico ( gabriela.vittistenico@uta.edu )
//
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "canvas/Utilities/Exception.h"

#include "sbndqm/Decode/PMT/PMTDecodeData/PMTDigitizerInfo.hh"
#include "sbndqm/Decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"
#include "sbndaq-online/helpers/Histogram.h"

// Add Fragment classes to import PTB data
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/TDCTimestampFragment.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/PTBFragment.hh"
#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
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

/************************************************************************************************************************/
/************************************** Instructions ********************************************************************/
/************************************************************************************************************************/

/******************************** November 17, 2023 ************************************************/
/**************************** Commissioning Phase: 0 ***********************************************/

/* PTB flash triggers: HLTs 0 to 5    */
/* PTB event triggers: HLTs 28 to 30  */
/* PTB CRT t1 resets:  HLTs 20 and 21 */

/***************************************************************************************************/
/***************************************************************************************************/

namespace sbndaq {
  class PTBdqm;
}

  class sbndaq::PTBdqm : public art::EDAnalyzer {

    public:

      explicit PTBdqm(fhicl::ParameterSet const & pset);
      virtual ~PTBdqm();
  
      virtual void analyze(art::Event const & evt) ;
  
    private:
    
      int fReportingLevel;
      int fBoardID;
      int fChannelNumber;
      
      int fEventBlock;
      int fBES;

      int eventcounter;
      int hlt_id;

    	void analyze_caen_fragment(artdaq::Fragment frag);
      void analyze_ptb_fragment(artdaq::Fragment frag);
      void analyze_tdc_fragment(artdaq::Fragment frag);
      
      void analyze_tdc_ptb();
      void resetdatavectors();
    	
      uint32_t nChannels;

	   std::vector<uint16_t>  fTicksVec;
	   std::vector< std::vector<uint16_t> > fWvfmsVec;

      std::vector<uint64_t> llt_trigger;
      std::vector<uint64_t> llt_ts;
      std::vector<uint64_t> hlt_trigger;
      std::vector<uint64_t> hlt_ts;

      std::vector<uint64_t> ftdc_ch0_utc;
      std::vector<uint64_t> ftdc_ch1_utc;
      std::vector<uint64_t> ftdc_ch2_utc;
      std::vector<uint64_t> ftdc_ch3_utc;
      std::vector<uint64_t> ftdc_ch4_utc;

      std::vector< std::vector<uint64_t> > llt_type_ts;
      std::vector< std::vector<uint64_t> > hlt_type_ts;

      std::vector<uint64_t> flash_trigger_ts;
      std::vector<uint64_t> event_trigger_ts;
      std::vector<uint64_t> crt_t1reset_ts;

  };


 // Define the constructor

sbndaq::PTBdqm::PTBdqm(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  //configuration
  fReportingLevel = pset.get<int>("ReportingLevel",0);
  fBoardID        = pset.get<int>("BoardID",3);
  fChannelNumber  = pset.get<int>("ChannelNumber",15);
  fEventBlock     = pset.get<int>("EventBlock",10);
  fBES            = pset.get<int>("BESGate",29);

  // Set event counter to zero
  eventcounter=0;

  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  
  //sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_trigger_rate"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_ptb_tdc_diff")); 
}

//------------------------------------------------------------------------------------------------------------------

 // Define the destructor
 sbndaq::PTBdqm::~PTBdqm()
 {
 }


//------------------------------------------------------------------------------------------------------------------


void sbndaq::PTBdqm::analyze_caen_fragment(artdaq::Fragment frag) {
          
       // GET WAVEFORM FROM CAEN CHANNEL "fChannelNumber", BOARD "fBoardID"
       // First, define the structure of a CAEN fragment
       CAENV1730Fragment bb(frag);

       CAENV1730Event const* event_ptr = bb.Event();
       CAENV1730EventHeader header     = event_ptr->Header;
       
       // specify boardID = fBoardID
       if(header.boardID == fBoardID){
       auto const* md = bb.Metadata();
       nChannels = md->nChannels;
       
       // Then, get information from the fragment header
       uint32_t ev_size_quad_bytes          = header.eventSize;
       uint32_t evt_header_size_quad_bytes  = sizeof(CAENV1730EventHeader)/sizeof(uint32_t);
       uint32_t data_size_double_bytes      = 2*(ev_size_quad_bytes - evt_header_size_quad_bytes);
       uint32_t wfm_length                  = data_size_double_bytes/nChannels;


       // Set where to start getting data through memory information
       const uint16_t* data_begin = reinterpret_cast<const uint16_t*>(frag.dataBeginBytes()
						      	 + sizeof(CAENV1730EventHeader));
       const uint16_t* value_ptr =  data_begin;
       uint16_t value = 0;
       size_t ch_offset = 0;

       std::string board_ID = std::to_string(header.boardID);

       // Specify channel = fChannelNumber
       size_t i_ch= fChannelNumber;

       fWvfmsVec[i_ch].resize(wfm_length);
       ch_offset = (size_t)(i_ch * wfm_length);
    

       // Loop over waveform samples
         for(size_t i_t=0; i_t<wfm_length; ++i_t){
            value_ptr = data_begin + ch_offset + i_t;
            value = *(value_ptr);
            fWvfmsVec[i_ch][i_t] = value;

            fTicksVec.push_back(fWvfmsVec[i_ch][i_t]);

         }
      // Checking if waveforms are printed correctly 
      // for(size_t g=0; g<fTicksVec.size(); g++){
      // std::cout << fTicksVec[g] << std::endl;
      // }


       double tickPeriod = 0.002; // [us]

       // send waveform from MSUM
       sbndaq::SendWaveform("snapshot:waveform:MSUM:" + board_ID, fTicksVec, tickPeriod);
       }

}

void sbndaq::PTBdqm::analyze_ptb_fragment(artdaq::Fragment frag) {

      CTBFragment ptb_fragment(frag);

      // Loop through all the PTB words in the fragment
      for ( size_t i = 0; i < ptb_fragment.NWords(); i++ ) {
    
         switch ( ptb_fragment.Word(i)->word_type ) {
    
            case 0x1 : // LL Trigger

               llt_trigger.emplace_back(round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2)) ); 
               llt_ts.emplace_back( ptb_fragment.TimeStamp(i) * 20 );

            break;
      
            case 0x2 : // HL Trigger

               hlt_id = round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2));

               hlt_trigger.emplace_back( hlt_id );
               // Assuming HLTs 28, 29 and 30 are flash triggers.
               if(hlt_id >= 28 && hlt_id <= 30 ){flash_trigger_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               // Assuming HLTs 0,1,2,3,4 and 5 are event triggers.
               if(hlt_id >= 0 && hlt_id <= 5 ){event_trigger_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               // Assuming HLTs 20 and 21 issue CRT t1 resets.
               if(hlt_id == 20 || hlt_id == 21){crt_t1reset_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               hlt_ts.emplace_back( ptb_fragment.TimeStamp(i) * 20 );

               
            break;
         }
      }

}

void sbndaq::PTBdqm::analyze_tdc_fragment(artdaq::Fragment frag) {

      TDCTimestampFragment tsfrag = TDCTimestampFragment(frag);
      const TDCTimestamp* ts = tsfrag.getTDCTimestamp();

      // CRT t1 reset 
      if (ts->vals.channel==0) {ftdc_ch0_utc.emplace_back(ts->timestamp_ns());}

      // BES 
      if (ts->vals.channel==1) {ftdc_ch1_utc.emplace_back(ts->timestamp_ns());}

      // RWM
      //  if (ts->vals.channel==2) {ftdc_ch2_utc.emplace_back(ts->timestamp_ns());}

      // PTB flash trigger
      if (ts->vals.channel==3) {ftdc_ch3_utc.emplace_back(ts->timestamp_ns());}

      // PTB event trigger
      if (ts->vals.channel==4) {ftdc_ch4_utc.emplace_back(ts->timestamp_ns());}

}

void sbndaq::PTBdqm::analyze_tdc_ptb() {

/**************************************************************************************************************************************/
/************************************** TRIGER RATES **********************************************************************************/
/**************************************************************************************************************************************/
      llt_type_ts.resize(32);
      hlt_type_ts.resize(32);
      
      // Calculate LLT and HLT rates. Loop over all possible IDs for Low- and High-Level triggers and get the correspondent timestamps
      for(size_t q=0; q<32; q++){

      
         // LLT rate     

         bool found_at_least_once_llt = false;
         auto start_it_llt = begin(llt_trigger);
         while (start_it_llt != end(llt_trigger)) {
            start_it_llt = std::find(start_it_llt, end(llt_trigger), q);
            if (start_it_llt != end(llt_trigger)) {
               auto const pos_llt = std::distance(begin(llt_trigger), start_it_llt);
               std::cout << "FOUND AT : " << pos_llt << '\n';
               ++start_it_llt;
               found_at_least_once_llt = true;
               auto llt_type_ts_ts = llt_type_ts[q].emplace_back(llt_ts[pos_llt]);
               std::cout << "TS VALUE : " << llt_type_ts_ts << '\n';
               std::cout << "LLT_TYPE SIZE: " << llt_type_ts[q].size() << '\n';
            }
         }
         
         if (!found_at_least_once_llt) {
              std::cout << "NOT FOUND" << '\n';
              continue;
         }  

         std::string lt_id = std::to_string(q);

         for(size_t r=0; r<llt_type_ts[q].size()-1; r++){
            sbndaq::sendMetric("LLT_ID", lt_id, "LLT_periodicity", /*1/*/((llt_type_ts[q][r+1]-llt_type_ts[q][r])*pow(10,-9)), fReportingLevel, artdaq::MetricMode::Average);

            std::cout << llt_type_ts[q].size()-1 << " "<< q << " " << r << " " << /*1/*/((llt_type_ts[q][r+1]-llt_type_ts[q][r])*pow(10,-9)) << std::endl; 
         
         
         }


      }

         // HLT rate
        
      for(size_t q=0; q<32; q++){

         bool found_at_least_once_hlt = false;
         auto start_it_hlt = begin(hlt_trigger);
         while (start_it_hlt != end(hlt_trigger)) {
            start_it_hlt = std::find(start_it_hlt, end(hlt_trigger), q);
            if (start_it_hlt != end(hlt_trigger)) {
               auto const pos_hlt = std::distance(begin(hlt_trigger), start_it_hlt);
               std::cout << "FOUND AT : " << pos_hlt << '\n';
               ++start_it_hlt;
               found_at_least_once_hlt = true;
               auto hlt_type_ts_ts = hlt_type_ts[q].emplace_back(hlt_ts[pos_hlt]);
               std::cout << "TS VALUE : " << hlt_type_ts_ts << '\n';
               std::cout << "HLT_TYPE SIZE: " << hlt_type_ts[q].size() << '\n';
               }
            }
         
            if (!found_at_least_once_hlt) {
               std::cout << "NOT FOUND" << '\n';
               continue;
            }  

         std::string ht_id = std::to_string(q);

         for(size_t s=0; s<hlt_type_ts[q].size()-1; s++){
            sbndaq::sendMetric("HLT_ID", ht_id, "HLT_periodicity", /*1/*/((hlt_type_ts[q][s+1]-hlt_type_ts[q][s])*pow(10,-9)), fReportingLevel, artdaq::MetricMode::Average);

            std::cout << hlt_type_ts[q].size()-1 << " "<< q << " " << s << " " << /*1/*/((hlt_type_ts[q][s+1]-hlt_type_ts[q][s])*pow(10,-9)) << std::endl; 
         }
      }

/**************************************************************************************************************************************/
/************************************** PTB - TDC timestamps **************************************************************************/
/**************************************************************************************************************************************/

      // Calculate PTB - TDC timestamp differences
      // Note that only positive metrics are being sent. 
      // Flash triggers and TDC channel 4.
      std::cout << "size TDC 4: " << ftdc_ch3_utc.size() << " size flash trig " << flash_trigger_ts.size() << std::endl;
      if(ftdc_ch3_utc.size() == flash_trigger_ts.size()) {
         
         for(size_t k=0; k<flash_trigger_ts.size(); k++){

            if(flash_trigger_ts[k]>=ftdc_ch3_utc[k]){
               sbndaq::sendMetric("FLASH_TDC_4", (flash_trigger_ts[k] - ftdc_ch3_utc[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|FLASH - TDC channel 4|: " << (flash_trigger_ts[k] - ftdc_ch3_utc[k])*0.001 << " microseconds" << std::endl;
            }
            else{
               sbndaq::sendMetric("FLASH_TDC_4", (ftdc_ch3_utc[k]-flash_trigger_ts[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|FLASH - TDC channel 4|: " << (ftdc_ch3_utc[k]-flash_trigger_ts[k])*0.001 << " microseconds" << std::endl;
            }

         }
      }
      else {sbndaq::sendMetric("NUMBER_TDC_4", ftdc_ch3_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_TDC_4: " << ftdc_ch3_utc.size() << std::endl;
            
            sbndaq::sendMetric("NUMBER_FLASH", flash_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_FLASH: " << flash_trigger_ts.size() << std::endl;
      }

      // Event triggers and TDC channel 5.
      if(ftdc_ch4_utc.size() == event_trigger_ts.size()) {
         
         for(size_t k=0; k<event_trigger_ts.size(); k++){

            if(event_trigger_ts[k]>=ftdc_ch4_utc[k]){
               sbndaq::sendMetric("EVENT_TDC_5", (event_trigger_ts[k] - ftdc_ch4_utc[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|EVENT - TDC channel 5|: " << (event_trigger_ts[k] - ftdc_ch4_utc[k])*0.001 << " microseconds" << std::endl;
            }
            else{
               sbndaq::sendMetric("EVENT_TDC_5", (ftdc_ch4_utc[k] - event_trigger_ts[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|EVENT - TDC channel 5|: " << (ftdc_ch4_utc[k] - event_trigger_ts[k])*0.001 << " microseconds" << std::endl;
            }

         }
      }
      else {sbndaq::sendMetric("NUMBER_TDC_5", ftdc_ch4_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_TDC_5: " << ftdc_ch4_utc.size() << std::endl;
            
            sbndaq::sendMetric("NUMBER_EVENT", event_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_EVENT: " << event_trigger_ts.size() << std::endl;
      }


      // CRT t1 reset and TDC channel 1.
      if(ftdc_ch0_utc.size() == crt_t1reset_ts.size()) {
         
         for(size_t k=0; k<crt_t1reset_ts.size(); k++){

            if(crt_t1reset_ts[k]>=ftdc_ch0_utc[k]){
               sbndaq::sendMetric("T1RESET_TDC_1", (crt_t1reset_ts[k] - ftdc_ch0_utc[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|T1RESET-TDC_1|: " << (crt_t1reset_ts[k] - ftdc_ch0_utc[k])*0.001 << " microseconds" << std::endl;
            }
            else{
               sbndaq::sendMetric("T1RESET_TDC_1", (ftdc_ch0_utc[k] - crt_t1reset_ts[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|T1RESET-TDC_1|: " << (ftdc_ch0_utc[k] - crt_t1reset_ts[k])*0.001 << " microseconds" << std::endl;
            }

         }
      }
      else {sbndaq::sendMetric("NUMBER_TDC_1", ftdc_ch0_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_TDC_1: " << ftdc_ch0_utc.size() << std::endl;
            
            sbndaq::sendMetric("NUMBER_T1RESET", crt_t1reset_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_T1RESET: " << crt_t1reset_ts.size() << std::endl;
      }

      // We assume the number of LLT related to BES and TDC BES is the same.
      std::cout << "size TDC 2: " << ftdc_ch1_utc.size() << " size BES trig " << llt_type_ts[fBES].size() << std::endl;
      if(llt_type_ts[fBES].size() == ftdc_ch1_utc.size()){

         for(size_t m=0; m<llt_type_ts[fBES].size(); m++){

            if(llt_type_ts[fBES][m]>=ftdc_ch1_utc[m]){
               sbndaq::sendMetric("LLTBES_TDC_2", (llt_type_ts[fBES][m] - ftdc_ch1_utc[m])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|LLT BES - TDC channel 2|: " << (llt_type_ts[fBES][m] - ftdc_ch1_utc[m])*0.001  << " microseconds" << std::endl;
            }
            else{
               sbndaq::sendMetric("LLTBES_TDC_2", (ftdc_ch1_utc[m] - llt_type_ts[fBES][m])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               std::cout << "|LLT BES - TDC channel 2|: " << (ftdc_ch1_utc[m] - llt_type_ts[fBES][m])*0.001  << " microseconds" << std::endl;
            }

         }
      }
      else {sbndaq::sendMetric("NUMBER_TDC_2", ftdc_ch1_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_TDC_2: " << ftdc_ch1_utc.size() << std::endl;
            
            sbndaq::sendMetric("NUMBER_BES", llt_type_ts[fBES].size(), fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "NUMBER_BES: " << llt_type_ts[fBES].size() << std::endl;
      }

}

void::sbndaq::PTBdqm::resetdatavectors(){
  
  // Reset data vectors
  // PTB
  llt_trigger.clear();
  llt_ts.clear();
  hlt_trigger.clear();
  hlt_ts.clear();

  llt_type_ts.clear();
  hlt_type_ts.clear();

  flash_trigger_ts.clear();
  event_trigger_ts.clear();
  crt_t1reset_ts.clear();

  // TDC
  ftdc_ch0_utc.clear();
  ftdc_ch1_utc.clear();
  ftdc_ch2_utc.clear();
  ftdc_ch3_utc.clear();
  ftdc_ch4_utc.clear();
}

void sbndaq::PTBdqm::analyze(art::Event const & evt) {

  // Print run and event information
  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;  
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event() << std::endl;

  // Clear CAEN data in the beginning of each event
  fTicksVec.clear();
  fWvfmsVec.clear();


  // Get the fragment information
  auto fragmentHandles = evt.getMany<artdaq::Fragments>();
  

  for (auto const& handle : fragmentHandles) {

    if (!handle.isValid() || handle->size() == 0) continue;

      for (auto const& frag : *handle){

         if(frag.type() == artdaq::Fragment::ContainerFragmentType) {
            
            artdaq::ContainerFragment cont_frag(frag);
            
            switch (cont_frag.fragment_type()){
                  
                  // Fragment from CAENV1730 - MTC/A MSUM waveform
                  case (sbndaq::detail::FragmentType::CAENV1730) : 
                     
                     fWvfmsVec.resize(16*handle->size());
                     
                     for (size_t ii = 0; ii < cont_frag.block_count(); ++ii){
                        
                        analyze_caen_fragment(*cont_frag[ii].get());
                        
                        // send EventMeta from MSUM
                        // Specify channel = fChannelNumber
                        size_t i_ch= fChannelNumber;
                        sbndaq::SendEventMeta("snapshot:waveform:MSUM:" + i_ch, evt);
                     
                      } break;    
  
                  // Fragment from PTB - LLT and HLT production
                  case (sbndaq::detail::FragmentType::PTB) : 
                                                   
                     for (size_t ii = 0; ii < cont_frag.block_count(); ++ii){
                        
                        analyze_ptb_fragment(*cont_frag[ii].get());
                       
                     
                     } break;

                  // Fragment from TDC - CRT t1 reset, BES, flash and event triggers
                  case (sbndaq::detail::FragmentType::TDCTIMESTAMP) : 
                                                   
                     for (size_t ii = 0; ii < cont_frag.block_count(); ++ii){
                        
                        analyze_tdc_fragment(*cont_frag[ii].get());
                       
                     
                     } break; 

	          }     

        }
      
        else {
            
            switch (handle->front().type()){ 
                  
                  // Fragment from CAENV1730 - MTC/A MSUM waveform
                  case (sbndaq::detail::FragmentType::CAENV1730) : { 
                     
                     fWvfmsVec.resize(16*handle->size());
                     analyze_caen_fragment(frag);
                     
                     // send EventMeta from MSUM
                     // Specify channel = fChannelNumber
                     size_t i_ch= fChannelNumber;
                     sbndaq::SendEventMeta("snapshot:waveform:MSUM:" + i_ch, evt);
                   
                  } break;
                  
                  // Fragment from PTB - LLT and HLT production
                  case (sbndaq::detail::FragmentType::PTB) : 
                  
                     analyze_ptb_fragment(frag); 
                     
                  break;

                  // Fragment from TDC - CRT t1 reset, BES, flash and event triggers
                  case (sbndaq::detail::FragmentType::TDCTIMESTAMP) : 
                  
                     analyze_tdc_fragment(frag); 
                     
                  break;

		
                   
            }
        
        }

    }//end loop over handle
  
  }//end loop over all handles

  std::cout << eventcounter << std::endl;

  // Set an event counter to controll data vector resets
  eventcounter++;

  // Impose data vector resetting after counting "fEventBlock" events
  if(eventcounter == fEventBlock){
  
  // After looping over "fEventBlock" events, calculate TDC - PTB timestamp differences
  // and LLT and HLT trigger rates
  analyze_tdc_ptb();
  
  resetdatavectors();

  eventcounter = 0;
  }

  // Include some text separation between events
  std::cout << std::endl;  
  std::cout << "######################################################################" << std::endl;

} // analyze

  

DEFINE_ART_MODULE(sbndaq::PTBdqm)