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

/* PTB event triggers: HLTs 0 to 19    */
/* PTB flash triggers: HLTs 22 to 30  */
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
      int fBBES;
      int fOBBES;
      int fLight;

      int eventcounter;
      int hlt_id;
      int llt_id;

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
//      std::vector<uint64_t> event_trigger_b_ts;
//      std::vector<uint64_t> event_trigger_ob_ts;
      std::vector<uint64_t> crt_t1reset_ts;
//      std::vector<uint64_t> crt_t1reset_b_ts;
//      std::vector<uint64_t> crt_t1reset_ob_ts;

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
  fBBES           = pset.get<int>("BeamBESGate",30);
  fOBBES          = pset.get<int>("OffBeamBESGate",26);
  fLight          = pset.get<int>("LightGate",14);

  // Set event counter to zero
  eventcounter=0;

  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  
  //sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_llt_trigger_rate"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_hlt_trigger_rate"));
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_beam_light_diff")); 
  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_beam_crt_diff")); 
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

               llt_id = round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2));
               //llt_trigger.emplace_back(round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2)) ); 
               llt_trigger.emplace_back(llt_id); 
               llt_ts.emplace_back( ptb_fragment.TimeStamp(i) * 20 );
               if(llt_id == fBBES || llt_id == fOBBES) std::cout << "BES LLT = " << llt_id << ", with timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;

            break;
      
            case 0x2 : // HL Trigger

               hlt_id = round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2));

               hlt_trigger.emplace_back( hlt_id );
               // Assuming HLTs 22 and above are flash triggers.
               if(hlt_id >= 22 && hlt_id <= 30 ){
                  flash_trigger_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
                  std::cout << "Flash found, HLT = " << hlt_id << ", with timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;
               }
               // Assuming HLTs 0 to 19 (inc) are event triggers.
               if(hlt_id >= 0 && hlt_id <= 19 ){
                  event_trigger_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
                  //std::cout << "Event found, HLT = " << hlt_id << ", with timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;
               }
               //if(hlt_id == 1 || hlt_id == 2 ){event_trigger_b_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               //if(hlt_id == 3 || hlt_id == 4 ){event_trigger_ob_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               // Assuming HLTs 20 and 21 issue CRT t1 resets.
               if(hlt_id == 20 || hlt_id == 21){
                  crt_t1reset_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
                  std::cout << "T1 Reset found, HLT = " << hlt_id << ", with timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;
               }
               //if(hlt_id == 20){crt_t1reset_b_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               //if(hlt_id == 21){crt_t1reset_ob_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               hlt_ts.emplace_back( ptb_fragment.TimeStamp(i) * 20 );

               
            break;
         }
      }

}

void sbndaq::PTBdqm::analyze_tdc_fragment(artdaq::Fragment frag) {

      TDCTimestampFragment tsfrag = TDCTimestampFragment(frag);
      const TDCTimestamp* ts = tsfrag.getTDCTimestamp();

      // CRT t1 reset 
      if (ts->vals.channel==0) {
         ftdc_ch0_utc.emplace_back(ts->timestamp_ns());
         std::cout << "TDC 1 with timestamp = " << ts->timestamp_ns() << std::endl;
      }

      // BES 
      if (ts->vals.channel==1) {
         ftdc_ch1_utc.emplace_back(ts->timestamp_ns());
         std::cout << "TDC 2 with timestamp = " << ts->timestamp_ns() << std::endl;
      }

      // RWM
      //  if (ts->vals.channel==2) {ftdc_ch2_utc.emplace_back(ts->timestamp_ns());}

      // PTB flash trigger
      if (ts->vals.channel==3) {
         ftdc_ch3_utc.emplace_back(ts->timestamp_ns());
         std::cout << "TDC 4 with timestamp = " << ts->timestamp_ns() << std::endl;
      }

      // PTB event trigger
      if (ts->vals.channel==4) {
         ftdc_ch4_utc.emplace_back(ts->timestamp_ns());
         std::cout << "TDC 5 with timestamp = " << ts->timestamp_ns() << std::endl;
      }

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
               //std::cout << "FOUND AT : " << pos_llt << '\n';
               ++start_it_llt;
               found_at_least_once_llt = true;
               llt_type_ts[q].emplace_back(llt_ts[pos_llt]);
               //auto llt_type_ts_ts = llt_type_ts[q].emplace_back(llt_ts[pos_llt]);
               //std::cout << "TS VALUE : " << llt_type_ts_ts << '\n';
               //std::cout << "LLT_TYPE SIZE: " << llt_type_ts[q].size() << '\n';
            }
         }
         std::string lt_id = std::to_string(q);
         if (!found_at_least_once_llt) {
              sbndaq::sendMetric("LLT_ID", lt_id, "LLT_periodicity", 0, fReportingLevel, artdaq::MetricMode::Average);
              //std::cout << "NOT FOUND" << '\n';
              continue;
         }  
         for(size_t r=0; r<llt_type_ts[q].size()-1; r++){
//            if ((llt_type_ts[q][r+1]-llt_type_ts[q][r])*pow(10,-9) > 2) std::cout << "Big llt periodicity: ID = " << lt_id << " val = "<< (llt_type_ts[q][r+1]-llt_type_ts[q][r])*pow(10,-9) << std::endl;
            sbndaq::sendMetric("LLT_ID", lt_id, "LLT_periodicity", /*1/*/((llt_type_ts[q][r+1]-llt_type_ts[q][r])*pow(10,-9)), fReportingLevel, artdaq::MetricMode::Average);
            //std::cout << llt_type_ts[q].size()-1 << " "<< q << " " << r << " " << /*1/*/((llt_type_ts[q][r+1]-llt_type_ts[q][r])*pow(10,-9)) << std::endl; 
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
               //std::cout << "HLT FOUND, HLT" << q << '\n';
               ++start_it_hlt;
               found_at_least_once_hlt = true;
               hlt_type_ts[q].emplace_back(hlt_ts[pos_hlt]);
               //auto hlt_type_ts_ts = hlt_type_ts[q].emplace_back(hlt_ts[pos_hlt]);
               //std::cout << "TS VALUE : " << hlt_type_ts_ts << '\n';
               //std::cout << "HLT_TYPE SIZE: " << hlt_type_ts[q].size() << '\n';
            }
         }
         std::string ht_id = std::to_string(q);
         if (!found_at_least_once_hlt) {
            sbndaq::sendMetric("HLT_ID", ht_id, "HLT_periodicity", 0, fReportingLevel, artdaq::MetricMode::Average);
            //std::cout << "NOT FOUND" << '\n';
            continue;
         }  
         for(size_t s=0; s<hlt_type_ts[q].size()-1; s++){
            std::cout << "hlt periodicity: ID = " << ht_id << " ts s+1 = " << hlt_type_ts[q][s+1] << " ts s = " << hlt_type_ts[q][s] << std::endl;
            std::cout << "diff = "<< (hlt_type_ts[q][s+1]-hlt_type_ts[q][s]) << std::endl;
            std::cout << "diff_s = "<< (hlt_type_ts[q][s+1]-hlt_type_ts[q][s])*pow(10,-9) << std::endl;
            sbndaq::sendMetric("HLT_ID", ht_id, "HLT_periodicity", 1/((hlt_type_ts[q][s+1]-hlt_type_ts[q][s])*pow(10,-9)), fReportingLevel, artdaq::MetricMode::Average);
            //std::cout << hlt_type_ts[q].size()-1 << " "<< q << " " << s << " " << /*1/*/((hlt_type_ts[q][s+1]-hlt_type_ts[q][s])*pow(10,-9)) << std::endl; 
         }
      }


/**************************************************************************************************************************************/
/************************************** PTB BES - light timestamp distribution ********************************************************/
/**************************************************************************************************************************************/

      //distribution of light triggers around BES (start of beam acceptance)
      size_t init_l = 0;
      for(size_t k=0; k<llt_type_ts[fBBES].size(); k++){
          bool inrange = false;
          for(size_t l = init_l; l < llt_type_ts[fLight].size(); l++){
              double diff = llt_type_ts[fBBES][k]-llt_type_ts[fLight][l];
              if(std::abs(diff) <= 10000){
                  inrange = true;
                  sbndaq::sendMetric("BEAM_LIGHT_DIFF","0","BEAM_LIGHT", diff, fReportingLevel, artdaq::MetricMode::Average);
              }else if(inrange){
                  init_l = l;
                  break;
              }
          }
      }

      //distribution of light triggers around offbeam BES (start of off beam acceptance)
      init_l = 0;
      for(size_t k=0; k<llt_type_ts[fOBBES].size(); k++){
          bool inrange = false;
          for(size_t l = init_l; l < llt_type_ts[fLight].size(); l++){
              double diff = llt_type_ts[fOBBES][k]-llt_type_ts[fLight][l];
              if(std::abs(diff) <= 10000){
                  inrange = true;
                  sbndaq::sendMetric("BEAM_LIGHT_DIFF","0","OFFBEAM_LIGHT", diff, fReportingLevel, artdaq::MetricMode::Average);
              }else if(inrange){
                  init_l = l;
                  break;
              }
          }
      }

/**************************************************************************************************************************************/
/************************************** PTB (off) beam Event - PTB CRT (off) beam Reset timestamps ************************************/
/**************************************************************************************************************************************/

      // Beam HLTs - Beam T1 Reset
      //std::cout << "size beam HLTs: " << event_trigger_b_ts.size() << ", size beam T1 resets " << crt_t1reset_b_ts.size() << std::endl;
      if(hlt_type_ts[1].size()+hlt_type_ts[2].size() == hlt_type_ts[20].size()) {
	  std::vector<uint64_t> hlt_beam_ts;
	  hlt_beam_ts.insert(hlt_beam_ts.end(), hlt_type_ts[1].begin(), hlt_type_ts[1].end());
	  hlt_beam_ts.insert(hlt_beam_ts.end(), hlt_type_ts[2].begin(), hlt_type_ts[2].end());
	  std::sort(hlt_beam_ts.begin(), hlt_beam_ts.end());
          for(size_t k=0; k<hlt_type_ts[20].size(); k++){
              sbndaq::sendMetric("BEAM_CRT_DIFF","0","BEAM_HLT_T1RESET", (hlt_beam_ts[k] - hlt_type_ts[20][k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
              //std::cout << "BEAM HLT - BEAM T1 RESET: " << (event_trigger_b_ts[k] - crt_t1reset_b_ts[k])*0.001 << " microseconds" << std::endl;
          }
      }else {
          sbndaq::sendMetric("BEAM_CRT_DIFF","1","NUMBER_BEAM_HLT", hlt_type_ts[1].size()+hlt_type_ts[2].size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_BEAM_HLT: " << event_trigger_b_ts.size() << std::endl;

          sbndaq::sendMetric("BEAM_CRT_DIFF","2","NUMBER_BEAM_T1RESET", hlt_type_ts[20].size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_BEAM_T1RESET: " << crt_t1reset_b_ts.size() << std::endl;
      }

      // Off Beam HLTs - Off Beam T1 Reset
      //std::cout << "size off beam HLTs: " << event_trigger_ob_ts.size() << ", size off beam T1 resets " << crt_t1reset_ob_ts.size() << std::endl;
      if(hlt_type_ts[3].size()+hlt_type_ts[4].size() == hlt_type_ts[21].size()) {
          std::vector<uint64_t> hlt_offbeam_ts;
          hlt_offbeam_ts.insert(hlt_offbeam_ts.end(), hlt_type_ts[3].begin(), hlt_type_ts[3].end());
          hlt_offbeam_ts.insert(hlt_offbeam_ts.end(), hlt_type_ts[4].begin(), hlt_type_ts[4].end());
          std::sort(hlt_offbeam_ts.begin(), hlt_offbeam_ts.end());
          for(size_t k=0; k<hlt_type_ts[21].size(); k++){
              sbndaq::sendMetric("BEAM_CRT_DIFF","0","OFFBEAM_HLT_T1RESET", (hlt_offbeam_ts[k] - hlt_type_ts[21][k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
              //std::cout << "OFFBEAM HLT - OFFBEAM T1 RESET: " << (event_trigger_ob_ts[k] - crt_t1reset_ob_ts[k])*0.001 << " microseconds" << std::endl;
          }
      }else {
          sbndaq::sendMetric("BEAM_CRT_DIFF","1","NUMBER_OFFBEAM_HLT", hlt_type_ts[3].size()+hlt_type_ts[4].size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_OFFBEAM_HLT: " << event_trigger_ob_ts.size() << std::endl;

          sbndaq::sendMetric("BEAM_CRT_DIFF","2","NUMBER_OFFBEAM_T1RESET", hlt_type_ts[21].size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_OFFBEAM_T1RESET: " << crt_t1reset_ob_ts.size() << std::endl;
      }

/**************************************************************************************************************************************/
/************************************** PTB - TDC timestamps **************************************************************************/
/**************************************************************************************************************************************/

      // Calculate PTB - TDC timestamp differences
      // Note that only positive metrics are being sent. 
      // Flash triggers and TDC channel 4.
      //std::cout << "size TDC 4: " << ftdc_ch3_utc.size() << " size flash trig " << flash_trigger_ts.size() << std::endl;
      if(ftdc_ch3_utc.size() == flash_trigger_ts.size()) {
         for(size_t k=0; k<flash_trigger_ts.size(); k++){
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC4_HLTFLASH", (ftdc_ch3_utc[k] - flash_trigger_ts[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
         }
      }else {
         bool match_found = false;
         size_t a = 0;
         size_t i_diff = 0;
         for(; a < flash_trigger_ts.size(); a++){
            for(size_t b = 0; b < ftdc_ch3_utc.size(); b++){
               uint64_t diff = flash_trigger_ts[a] - ftdc_ch3_utc[b];
               double diff_us = diff*0.001;
               if(diff_us < 1){
                  i_diff = a - b;
                  match_found = true;
                  break;
               }
            }
            if(match_found) break;
         }
         if(match_found){
            for(size_t i = a; i < flash_trigger_ts.size(); i++){
               if(i-i_diff > ftdc_ch3_utc.size()) break;
               sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC4_HLTFLASH", (ftdc_ch3_utc[i-i_diff] - flash_trigger_ts[i])*0.001, fReportingLevel, artdaq::MetricMode::Average);
            }
         }else{
            sbndaq::sendMetric("PTB_TDC_DIFF","2","NUMBER_TDC4", ftdc_ch3_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            sbndaq::sendMetric("PTB_TDC_DIFF","1","NUMBER_FLASH", flash_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
         }
      }

      // Event triggers and TDC channel 5.
      if(ftdc_ch4_utc.size() == event_trigger_ts.size()) {
         //std::cout << "#tdc5 = #event = " << event_trigger_ts.size() << std::endl;
         for(size_t k=0; k<event_trigger_ts.size(); k++){
	    //std::cout << "Event TS = " << event_trigger_ts[k] << ", and TDC5 TS = " << ftdc_ch4_utc[k] <<std::endl;
	    //std::cout << "DIFF = " <<  (ftdc_ch4_utc[k] - event_trigger_ts[k])*0.001 << std::endl;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC5_HLTEVENT", (ftdc_ch4_utc[k] - event_trigger_ts[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
            // if this sends a stupdily big number it means tdc<hlt (uint cant do -ves)and that's very wrong, add alarm about it
         }
      }else {
         //std::cout << "#event = " << event_trigger_ts.size() << std::endl;
         //std::cout << "#tdc5 = " << ftdc_ch4_utc.size() << std::endl;
         bool match_found = false;
         size_t a = 0;
         size_t i_diff = 0;
         for(; a < event_trigger_ts.size(); a++){
            for(size_t b = 0; b < ftdc_ch4_utc.size(); b++){
               uint64_t diff = ftdc_ch4_utc[b] - event_trigger_ts[a];
               double diff_us = diff*0.001;
               if(diff_us < 1){
                  i_diff = a - b;
                  match_found = true;
		  //std::cout << "Found matching index for event-TDC5 with a = " << a << ", and b = " << b <<std::endl;
		  //std::cout << "TDC5 TS = " << ftdc_ch4_utc[b] << " and HLT Event TS = " << event_trigger_ts[a] <<std::endl;
                  break;
               }
            }
            if(match_found) break;
         }
         if(match_found){
            size_t shift_tdc = 0;
            size_t shift_ptb = 0;
            for(size_t i = a; i < event_trigger_ts.size(); i++){
               if(i-i_diff+shift_tdc > ftdc_ch4_utc.size()) break;
               uint64_t diff;
               double diff_us;
               if(event_trigger_ts[i+shift_ptb] < ftdc_ch4_utc[i-i_diff+shift_tdc]){
	          diff = ftdc_ch4_utc[i-i_diff+shift_tdc] - event_trigger_ts[i+shift_ptb];
                  diff_us = diff*0.001; //want positive when event ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
                                        //could leave it with the unsigned int error not copeing with negatvies adn going into underflow (very large numebr) to highlight this
                  if(diff_us > 1){
                     std::cout << "diff tdc - hlt event more than 1 us" <<std::endl;
                     uint64_t diff_test = ftdc_ch4_utc[i-i_diff+shift_tdc+1] - event_trigger_ts[i+shift_ptb];
                     if (diff_test < 1000){
                        shift_tdc = shift_tdc + 1;
                        diff_us = diff_test*0.001;
                        std::cout << "skipping a tdc timestamp" <<std::endl;
                     }else {
                        diff_test = ftdc_ch4_utc[i-i_diff+shift_tdc] - event_trigger_ts[i+shift_ptb+1];
                        if (diff_test < 1000){
                           shift_ptb = shift_ptb + 1;
                           diff_us = diff_test*0.001;
                           std::cout << "skipping a hlt timestamp" <<std::endl;
                        }else {
                           std::cout << "No updated match found for indices tdc5-hlt"<< std::endl;
                           diff_us = 999999999999; //big number so alarms
                           //break;
                        }
                     }
                  }
               } else{
                  diff_us = 999999999999; //big number so alarms
               }
               //sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC5-HLTEVENT", (event_trigger_ts[i] - ftdc_ch4_utc[i-i_diff])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC5_HLTEVENT", diff_us, fReportingLevel, artdaq::MetricMode::Average);
	       //std::cout << "TDC5 TS = " << ftdc_ch4_utc[i-i_diff] << " and HLT Event TS = " << event_trigger_ts[i] << std::endl;
	       //std::cout << "DIFF = " << diff_us << std::endl;
            }
         }else {
            sbndaq::sendMetric("PTB_TDC_DIFF","2","NUMBER_TDC5", ftdc_ch4_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            sbndaq::sendMetric("PTB_TDC_DIFF","1","NUMBER_EVENT", event_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
         }
      }


      // CRT t1 reset and TDC channel 1.
      if(ftdc_ch0_utc.size() == crt_t1reset_ts.size()) {
         //std::cout<< "#t1reset == #tdc1 == "<< crt_t1reset_ts.size() <<std::endl;
         for(size_t k=0; k<crt_t1reset_ts.size(); k++){
	    uint64_t diff;
	    double diff_us;
	    if(crt_t1reset_ts[k] < ftdc_ch0_utc[k]){
	       diff = ftdc_ch0_utc[k] - crt_t1reset_ts[k];
	       diff_us = diff*0.001;
	    }else{
               diff_us = 999999999999; //big number so alarms
	    }
	    //std::cout << "TDC1 TS = " << ftdc_ch0_utc[k] << " and HLT T1 TS = " << crt_t1reset_ts[k] << std::endl;
	    //std::cout << "DIFF = " << diff_us << std::endl;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC1_HLTT1", diff_us, fReportingLevel, artdaq::MetricMode::Average);
            //sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC1-HLTT1", (crt_t1reset_ts[k] - ftdc_ch0_utc[k])*0.001, fReportingLevel, artdaq::MetricMode::Average);
         }
      }else {
         bool match_found = false;
         size_t a = 0;
         size_t i_diff = 0;
         for(; a < crt_t1reset_ts.size(); a++){
            for(size_t b = 0; b < ftdc_ch0_utc.size(); b++){
               uint64_t diff = ftdc_ch0_utc[b] - crt_t1reset_ts[a];
               double diff_us = diff*0.001;
               if(diff_us < 1){
                  i_diff = a - b;
                  match_found = true;
		  //std::cout << "Found matching index for crt_t1reset(hlt 20 or 21) - TDC1 with a = " << a << ", and b = " << b <<std::endl;
		  //std::cout << "CRT T1 Reset = " << crt_t1reset_ts[a] << ", and TDC1 TS = " << ftdc_ch0_utc[b] <<std::endl;
                  break;
               }
            }
            if(match_found) break;
         }
         if(match_found){
            for(size_t i = a; i < crt_t1reset_ts.size(); i++){
               if(i-i_diff > ftdc_ch0_utc.size()) break;
               uint64_t diff;
               double diff_us;
               if(crt_t1reset_ts[i] < ftdc_ch0_utc[i-i_diff]){
	          diff = ftdc_ch0_utc[i-i_diff] - crt_t1reset_ts[i];
                  diff_us = diff*0.001;
               } else{
                  diff_us = 999999999999; //big number so alarms
               }
	       //std::cout << "PTB T1 TS = " << crt_t1reset_ts[i] << ", and TDC1 TS = " << ftdc_ch0_utc[i-i_diff] <<std::endl;
	       //std::cout << "DIFF = " << diff_us << std::endl;
               //sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC1-HLTT1", (crt_t1reset_ts[i] - ftdc_ch0_utc[i-i_diff])*0.001, fReportingLevel, artdaq::MetricMode::Average);
               sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC1-HLTT1", diff_us, fReportingLevel, artdaq::MetricMode::Average);
            }
         }else {
            sbndaq::sendMetric("PTB_TDC_DIFF","2","NUMBER_TDC1", ftdc_ch0_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            sbndaq::sendMetric("PTB_TDC_DIFF","1","NUMBER_T1RESET", crt_t1reset_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
         }
      }

      // We assume the number of LLT related to BES and TDC BES is the same.
      //std::cout << "size TDC 2: " << ftdc_ch1_utc.size() << " size BES trig " << llt_type_ts[fBBES].size() << std::endl;
      if(llt_type_ts[fOBBES].size() == ftdc_ch1_utc.size()){
         for(size_t m=0; m<llt_type_ts[fOBBES].size(); m++){
	    uint64_t diff;
	    double diff_us;
	    if(llt_type_ts[fOBBES][m] < ftdc_ch1_utc[m]){
	       diff = ftdc_ch1_utc[m] - llt_type_ts[fOBBES][m];
	       diff_us = diff*0.001;
	    }else{
               diff_us = 999999999999; //big number so alarms
	    }
	    std::cout << "TDC2 TS = " << ftdc_ch0_utc[m] << " and LLT BES TS = " << llt_type_ts[fOBBES][m] << std::endl;
	    std::cout << "DIFF = " << diff_us << std::endl;
            //sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC2_LLTBES", (ftdc_ch1_utc[m] - llt_type_ts[fOBBES][m])*0.001, fReportingLevel, artdaq::MetricMode::Average);
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC2_LLTBES", diff_us, fReportingLevel, artdaq::MetricMode::Average);
         }
      }else {
         bool match_found = false;
         size_t a = 0;
         size_t i_diff = 0;
         for(; a < llt_type_ts[fOBBES].size(); a++){
            for(size_t b = 0; b < ftdc_ch1_utc.size(); b++){
               uint64_t diff = ftdc_ch1_utc[b] - llt_type_ts[fOBBES][a];
               double diff_us = diff*0.001;
               if(diff_us < 1){
                  i_diff = a - b;
                  match_found = true;
                  break;
               }
            }
            if(match_found) break;
         }
         if(match_found){
            for(size_t i = a; i < llt_type_ts[fOBBES].size(); i++){
               if(i-i_diff > ftdc_ch1_utc.size()) break;
               sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC2_LLTBES", (ftdc_ch1_utc[i-i_diff] - llt_type_ts[fOBBES][i])*0.001, fReportingLevel, artdaq::MetricMode::Average);
            }
         }else {
            sbndaq::sendMetric("PTB_TDC_DIFF","2","NUMBER_TDC2", ftdc_ch1_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
            sbndaq::sendMetric("PTB_TDC_DIFF","1","NUMBER_BES", llt_type_ts[fOBBES].size(), fReportingLevel, artdaq::MetricMode::Average);
         }
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
  //event_trigger_b_ts.clear();
  //event_trigger_ob_ts.clear();
  crt_t1reset_ts.clear();
  //crt_t1reset_b_ts.clear();
  //crt_t1reset_ob_ts.clear();

  // TDC
  ftdc_ch0_utc.clear();
  ftdc_ch1_utc.clear();
  ftdc_ch2_utc.clear();
  ftdc_ch3_utc.clear();
  ftdc_ch4_utc.clear();
}

void sbndaq::PTBdqm::analyze(art::Event const & evt) {

  // Print run and event information
  //std::cout << "######################################################################" << std::endl;
  //std::cout << std::endl;  
  //std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event() << std::endl;

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

  //std::cout << eventcounter << std::endl;

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
  //std::cout << std::endl;  
  //std::cout << "######################################################################" << std::endl;

} // analyze

  

DEFINE_ART_MODULE(sbndaq::PTBdqm)
