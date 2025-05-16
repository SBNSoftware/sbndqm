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
#include <sys/resource.h>

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

      std::vector<art::Handle<artdaq::Fragments>> readHandles( art::Event const & evt ) const;
  
    private:
    
      std::vector<std::string> m_input_tags;

      int fReportingLevel;
      int fBoardID;
      int fChannelNumber;
      
      int fEventBlock;
      int fBBES;
      int fOBBES;
      int fLight;
      int fTDCT1;
      int fTDCBES;
      int fTDCFlash;
      int fTDCEvent;

      int eventcounter;
      int ptbcontcounter;
      int ptbfragcounter;
      int hlt_id;
      int llt_id;

      void analyze_caen_fragment(artdaq::Fragment frag);
      void analyze_ptb_fragment(artdaq::Fragment frag, int eventcounter);
      void analyze_tdc_fragment(artdaq::Fragment frag);
      void analyze_trigger_rates();
      void analyze_event_metrics();
      void resetdatavectors();
      void reseteventdatavectors();
      static bool sortcol( const std::vector<uint64_t>& v1, const std::vector<uint64_t>& v2 );	
      uint32_t nChannels;

      bool ptb_cont_frag;

      std::vector<uint16_t>  fTicksVec;
      std::vector< std::vector<uint16_t> > fWvfmsVec;


      std::vector<uint64_t> llt_light_ts;
      std::vector<uint64_t> llt_bbes_ts;
      std::vector<uint64_t> llt_obbes_ts;

      std::vector<uint64_t> ftdc_t1_utc;
      std::vector<uint64_t> ftdc_bes_utc;
      //std::vector<uint64_t> ftdc_ch2_utc;
      std::vector<uint64_t> ftdc_flash_utc;
      std::vector<uint64_t> ftdc_event_utc;

      std::string fDAQLabel;
      std::string fPTBContainerInstance;

      std::vector< std::vector<uint64_t> > llt_type_ts;
      std::vector< std::vector<uint64_t> > hlt_type_ts;
      std::vector< std::vector<uint64_t> > hlt_counts;
      std::vector< std::vector<uint64_t> > llt_counts;

      std::vector<uint64_t> flash_trigger_ts;
      std::vector<uint64_t> event_trigger_ts;
      std::vector<uint64_t> hlt_beam_ts;
      std::vector<uint64_t> hlt_offbeam_ts;
      std::vector<uint64_t> crt_t1reset_ts;
      std::vector<uint64_t> crt_t1reset_b_ts;
      std::vector<uint64_t> crt_t1reset_ob_ts;

  };


 // Define the constructor

sbndaq::PTBdqm::PTBdqm(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  //configuration
  fReportingLevel = pset.get<int>("ReportingLevel",0);
  fBoardID        = pset.get<int>("BoardID",0);
  fChannelNumber  = pset.get<int>("ChannelNumber",3);
  fEventBlock     = pset.get<int>("EventBlock",1);
  fBBES           = pset.get<int>("BeamBESGate",30);
  fOBBES          = pset.get<int>("OffBeamBESGate",26);
  fLight          = pset.get<int>("LightGate",20);
  fTDCT1          = pset.get<int>("TDCT1",0);
  fTDCBES         = pset.get<int>("TDCBES",1);
  fTDCFlash       = pset.get<int>("TDCFlash",3);
  fTDCEvent       = pset.get<int>("TDCEvent",4);
  fDAQLabel       = pset.get<std::string>("DAQLabel", "daq");
  fPTBContainerInstance = pset.get<std::string>("PTBContainerInstance", "ContainerPTB"); 

  // Set event counter to zero
  eventcounter=0;
  ptbcontcounter=0;
  ptbfragcounter=0;
  ptb_cont_frag = true;
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

std::vector<art::Handle<artdaq::Fragments>> sbndaq::PTBdqm::readHandles( art::Event const & event ) const{
  std::vector<art::Handle<artdaq::Fragments>> handles;
  std::vector<std::string>  m_input_tags = { "CAENV1730", "ContainerCAENV1730", "ContainerPTB", "PTB", "ContainerTDCTIMESTAMP", "TDCTIMESTAMP"};
  for( std::string const& input_tag : m_input_tags ) {
    art::Handle<artdaq::Fragments> thisHandle;
    event.getByLabel("daq", input_tag, thisHandle);
    if( !thisHandle.isValid() || thisHandle->empty() ) continue;
    handles.push_back( thisHandle );
  }
  return handles;
}

//------------------------------------------------------------------------------------------------------------------
bool sbndaq::PTBdqm::sortcol( const std::vector<uint64_t>& v1,
               const std::vector<uint64_t>& v2 ) {
    return v1[0] < v2[0];
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
	       double tickPeriod = 0.002; // [us]
	       // send waveform from MSUM
	       sbndaq::SendWaveform("snapshot:waveform:MSUM:" + board_ID, fTicksVec, tickPeriod);
       }

}

void sbndaq::PTBdqm::analyze_ptb_fragment(artdaq::Fragment frag, int eventcounter) {
      CTBFragment ptb_fragment(frag);

      hlt_type_ts.resize(32);
      hlt_counts.resize(fEventBlock);
      llt_counts.resize(fEventBlock);
      for(int i =0; i<fEventBlock;++i){
         hlt_counts[i].resize(33);
         llt_counts[i].resize(33);
      }
      llt_type_ts.resize(32);

      for ( size_t i = 0; i < ptb_fragment.NWords(); i++ ) {
          
         switch ( ptb_fragment.Word(i)->word_type ) {
    
            case 0x1 : // LL Trigger
            {
               uint64_t trigger_mask = ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF;
               
               //gave multiple triggers of same type in the same tick
	       while (trigger_mask) {
		   size_t lt_id = __builtin_ctzll(trigger_mask); // Get the least significant 1-bit index
		   trigger_mask &= (trigger_mask - 1); // Turn off the least significant 1-bit
                   int llt_id = static_cast<int>(lt_id);

		   if (llt_id == fBBES)                     llt_bbes_ts.emplace_back(frag.timestamp());
		   else if (llt_id == fOBBES)               llt_obbes_ts.emplace_back(frag.timestamp());
                   else if (llt_id == fLight)               llt_light_ts.emplace_back(frag.timestamp());

		   llt_type_ts[llt_id].emplace_back(frag.timestamp());
		   ++llt_counts[eventcounter][llt_id+1]; 

	       }
               break;
            }
      
            case 0x2 : // HL Trigger
            {
               uint64_t trigger_mask = ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF;
               while (trigger_mask) {
                   size_t ht_id = __builtin_ctzll(trigger_mask); // Get the least significant 1-bit index
                   trigger_mask &= (trigger_mask - 1); // Turn off the least significant 1-bit
                   int hlt_id = static_cast<int>(ht_id);
                   if(hlt_id < 32){
		      hlt_type_ts[hlt_id].emplace_back(frag.timestamp());
		      if(hlt_id >= 22 && hlt_id <= 30 ){
			 flash_trigger_ts.emplace_back(frag.timestamp());
		      }
		      else if(hlt_id == 20 || hlt_id == 21){
			 crt_t1reset_ts.emplace_back(frag.timestamp());
                         if(hlt_id == 20) crt_t1reset_b_ts.emplace_back(frag.timestamp());
                         else             crt_t1reset_ob_ts.emplace_back(frag.timestamp());
		      }
		      else if(hlt_id >= 0 && hlt_id <= 19 ){
			 event_trigger_ts.emplace_back(frag.timestamp());
		      }
		      if(hlt_id == 1 || hlt_id == 2 ){
			 hlt_beam_ts.emplace_back(frag.timestamp());
		      }
		      else if(hlt_id == 3 || hlt_id == 4 ){
			 hlt_offbeam_ts.emplace_back(frag.timestamp());
		      }
		      ++hlt_counts[eventcounter][hlt_id+1];
                   }
               }
               break;
             }
         }
      }
}

void sbndaq::PTBdqm::analyze_tdc_fragment(artdaq::Fragment frag) {

      TDCTimestampFragment tsfrag = TDCTimestampFragment(frag);
      const TDCTimestamp* ts = tsfrag.getTDCTimestamp();
      // CRT t1 reset 
      if (ts->vals.channel==fTDCT1) {
         ftdc_t1_utc.emplace_back(ts->timestamp_ns());
      }

      // BES 
      if (ts->vals.channel==fTDCBES) {
         ftdc_bes_utc.emplace_back(ts->timestamp_ns());
      }

      // RWM
      //  if (ts->vals.channel==2) {ftdc_ch2_utc.emplace_back(ts->timestamp_ns());}

      // PTB flash trigger
      if (ts->vals.channel==fTDCFlash) {
         ftdc_flash_utc.emplace_back(ts->timestamp_ns());
      }

      // PTB event trigger
      if (ts->vals.channel==fTDCEvent) {
         ftdc_event_utc.emplace_back(ts->timestamp_ns());
      }

}

void sbndaq::PTBdqm::analyze_trigger_rates() {
      sort(hlt_counts.begin(),hlt_counts.end(),sortcol);
      sort(llt_counts.begin(),llt_counts.end(),sortcol);
/**************************************************************************************************************************************/
/************************************** TRIGER RATES **********************************************************************************/
/**************************************************************************************************************************************/
      for(size_t q=0; q<32; q++){
         std::string lt_id = std::to_string(q);
         //std::cout << "Found LLT " << lt_id << " entries = " << llt_type_ts[q].size() << std::endl;
         sbndaq::sendMetric("LLT_ID", lt_id, "LLT_counts", llt_type_ts[q].size(), fReportingLevel, artdaq::MetricMode::LastPoint);
      }

      for(size_t q=0; q<32; q++){
         std::string ht_id = std::to_string(q);
         sbndaq::sendMetric("HLT_ID", ht_id, "HLT_counts", hlt_type_ts[q].size(), fReportingLevel, artdaq::MetricMode::LastPoint);
      }
    }

void sbndaq::PTBdqm::analyze_event_metrics() {


/**************************************************************************************************************************************/
/************************************** PTB BES - light timestamp distribution ********************************************************/
/**************************************************************************************************************************************/

      //distribution of light triggers around BES (start of beam acceptance)
	  for(size_t k=0; k<llt_bbes_ts.size(); k++){
	      for(size_t l = 0; l < llt_light_ts.size(); l++){
		uint64_t diff;
		double diff_sign;
		if (llt_bbes_ts[k] > llt_light_ts[l]){
		   diff = llt_bbes_ts[k]-llt_light_ts[l];
		   diff_sign = -0.001 * diff; //want -ve when ligth before bes, us
		} else {
		   diff = llt_light_ts[l] - llt_bbes_ts[k]; //want +ve when ligth after bes
		   diff_sign = diff*0.001; //us
		}
		sbndaq::sendMetric("BEAM_LIGHT_DIFF","0","BEAM_LIGHT", diff_sign, fReportingLevel, artdaq::MetricMode::Average);
	      }
	  }

      //distribution of light triggers around offbeam BES (start of off beam acceptance)
	  for(size_t k=0; k<llt_obbes_ts.size(); k++){
	      for(size_t l = 0; l < llt_light_ts.size(); l++){
		  uint64_t diff;
		  double diff_sign;
		  if (llt_obbes_ts[k] > llt_light_ts[l]){
		     diff = llt_obbes_ts[k]-llt_light_ts[l];
		     diff_sign = -0.001 * diff; //want -ve when light before bes
		  } else {
		     diff = llt_light_ts[l] - llt_obbes_ts[k]; //want +ve when light after bes
		     diff_sign = diff * 0.001;
		  }
		  sbndaq::sendMetric("BEAM_LIGHT_DIFF","0","OFFBEAM_LIGHT", diff_sign, fReportingLevel, artdaq::MetricMode::Average);
	      }
	  }
/**************************************************************************************************************************************/
/************************************** PTB (off) beam Event - PTB CRT (off) beam Reset timestamps ************************************/
/**************************************************************************************************************************************/

      // Beam HLTs - Beam T1 Reset
      if(hlt_beam_ts.size() == crt_t1reset_b_ts.size()) {
          std::sort(hlt_beam_ts.begin(), hlt_beam_ts.end());
          std::sort(crt_t1reset_b_ts.begin(), crt_t1reset_b_ts.end());
          for(size_t k=0; k<crt_t1reset_b_ts.size(); k++){
              if (hlt_beam_ts[k] > crt_t1reset_b_ts[k]){
		  uint64_t diff = hlt_beam_ts[k] - crt_t1reset_b_ts[k];
		  double diff_us = diff * 0.001;
		  sbndaq::sendMetric("BEAM_CRT_DIFF","0","BEAM_HLT_T1RESET", diff_us, fReportingLevel, artdaq::MetricMode::Average);
              } else{
		  uint64_t diff = crt_t1reset_b_ts[k] - hlt_beam_ts[k];
		  double diff_us = diff * -0.001;
		  sbndaq::sendMetric("BEAM_CRT_DIFF","0","BEAM_HLT_T1RESET", diff_us, fReportingLevel, artdaq::MetricMode::Average);
              }
          } 
      }
      sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_BEAM_HLT", hlt_beam_ts.size(), fReportingLevel, artdaq::MetricMode::Average);

      sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_BEAM_T1RESET", crt_t1reset_b_ts.size(), fReportingLevel, artdaq::MetricMode::Average);

      // Off Beam HLTs - Off Beam T1 Reset
      if(hlt_offbeam_ts.size() == crt_t1reset_ob_ts.size()) {
          std::sort(hlt_offbeam_ts.begin(), hlt_offbeam_ts.end());
          std::sort(crt_t1reset_ob_ts.begin(), crt_t1reset_ob_ts.end());
          for(size_t k=0; k<crt_t1reset_ob_ts.size(); k++){
              if(hlt_offbeam_ts[k] > crt_t1reset_ob_ts[k]){
                  uint64_t diff = hlt_offbeam_ts[k] - crt_t1reset_ob_ts[k];
                  double diff_us = diff*0.001;
                  sbndaq::sendMetric("BEAM_CRT_DIFF","0","OFFBEAM_HLT_T1RESET", diff_us, fReportingLevel, artdaq::MetricMode::Average);
              }
              else{
		  uint64_t diff = crt_t1reset_ob_ts[k] - hlt_offbeam_ts[k];
		  double diff_us = diff * -0.001;
		  sbndaq::sendMetric("BEAM_CRT_DIFF","0","OFFBEAM_HLT_T1RESET", diff_us, fReportingLevel, artdaq::MetricMode::Average);
              }
          }
      }
      sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_OFFBEAM_HLT", hlt_offbeam_ts.size(), fReportingLevel, artdaq::MetricMode::Average);

      sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_OFFBEAM_T1RESET", crt_t1reset_ob_ts.size(), fReportingLevel, artdaq::MetricMode::Average);

/**************************************************************************************************************************************/
/************************************** PTB - TDC timestamps **************************************************************************/
/**************************************************************************************************************************************/

      // Calculate PTB - TDC timestamp differences
      sort(ftdc_t1_utc.begin(), ftdc_t1_utc.end());
      sort(ftdc_bes_utc.begin(), ftdc_bes_utc.end());
      sort(ftdc_flash_utc.begin(), ftdc_flash_utc.end());
      sort(ftdc_event_utc.begin(), ftdc_event_utc.end());
      sort(llt_bbes_ts.begin(), llt_bbes_ts.end());
      sort(flash_trigger_ts.begin(), flash_trigger_ts.end());
      sort(event_trigger_ts.begin(), event_trigger_ts.end());
      sort(crt_t1reset_ts.begin(), crt_t1reset_ts.end());

      // Flash triggers and TDC channel 4.
      if(ftdc_flash_utc.size() == flash_trigger_ts.size()) {
         for(size_t k=0; k<flash_trigger_ts.size(); k++){
            uint64_t diff;
            double diff_us;
            if(ftdc_flash_utc[k] > flash_trigger_ts[k]){
               diff = ftdc_flash_utc[k] - flash_trigger_ts[k];
               diff_us = diff * 0.001;
            }else {
               diff = flash_trigger_ts[k] - ftdc_flash_utc[k];
               diff_us = diff * -0.001;
            }
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_HLT_FLASH", diff_us, fReportingLevel, artdaq::MetricMode::Average);
         }
      }
      
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_FLASH", ftdc_flash_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_FLASH", flash_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);

      // Event triggers and TDC channel 5.
      if(ftdc_event_utc.size() == 1 && event_trigger_ts.size() > 0) { //do 1st event trigger ts only
	  uint64_t diff;
	  double diff_us;
	  if(ftdc_event_utc[0] > event_trigger_ts[0]){
	     diff = ftdc_event_utc[0] - event_trigger_ts[0];
	     diff_us = diff*0.001;
	  } else {
	     diff = event_trigger_ts[0] - ftdc_event_utc[0];
	     diff_us = diff * -0.001;
	  }
	  sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_HLT_EVENT", diff_us, fReportingLevel, artdaq::MetricMode::Average);
      }
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_EVENT", ftdc_event_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_EVENT", event_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);

      // CRT t1 reset and TDC channel 1.
      if(ftdc_t1_utc.size() == crt_t1reset_ts.size()) {
         for(size_t k=0; k<crt_t1reset_ts.size(); k++){
	    uint64_t diff;
	    double diff_us;
	    if(crt_t1reset_ts[k] < ftdc_t1_utc[k]){
	       diff = ftdc_t1_utc[k] - crt_t1reset_ts[k];
	       diff_us = diff*0.001;
	    }else{
               diff = crt_t1reset_ts[k] - ftdc_t1_utc[k];
               diff_us = diff * -0.001;
	    }
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_HLT_T1", diff_us, fReportingLevel, artdaq::MetricMode::Average);
         }
      }
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_T1", ftdc_t1_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_T1", crt_t1reset_ts.size(), fReportingLevel, artdaq::MetricMode::Average);

      // TDC - LLT BES
      if(llt_bbes_ts.size() == ftdc_bes_utc.size()){
         for(size_t k=0; k<llt_bbes_ts.size(); k++){
	    uint64_t diff;
	    double diff_us;
	    if(llt_bbes_ts[k] < ftdc_bes_utc[k]){
	       diff = ftdc_bes_utc[k] - llt_bbes_ts[k];
	       diff_us = diff*0.001;
	    }else{
	       diff = llt_bbes_ts[k] - ftdc_bes_utc[k];
	       diff_us = diff*-0.001;
	    }
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_LLT_BES", diff_us, fReportingLevel, artdaq::MetricMode::Average);
         }
      }
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_BES", ftdc_bes_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
      sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_BES", llt_bbes_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
}


void::sbndaq::PTBdqm::resetdatavectors(){

  llt_type_ts.clear();
  hlt_type_ts.clear();
  hlt_counts.clear();
  llt_counts.clear();

}

void::sbndaq::PTBdqm::reseteventdatavectors(){
  
  // Reset data vectors

  // PTB
  llt_obbes_ts.clear();
  llt_bbes_ts.clear();
  llt_light_ts.clear();

  flash_trigger_ts.clear();
  event_trigger_ts.clear();
  hlt_beam_ts.clear();
  hlt_offbeam_ts.clear();
  crt_t1reset_ts.clear();
  crt_t1reset_b_ts.clear();
  crt_t1reset_ob_ts.clear();

  // TDC
  ftdc_t1_utc.clear();
  ftdc_bes_utc.clear();
  //ftdc_ch2_utc.clear();
  ftdc_flash_utc.clear();
  ftdc_event_utc.clear();
}

void sbndaq::PTBdqm::analyze(art::Event const & evt) {

  // Clear CAEN data in the beginning of each event
  fTicksVec.clear();
  fWvfmsVec.clear();
  
  //Get PTB fragment container
  art::InputTag itag(fDAQLabel, fPTBContainerInstance);
  auto cont_frags = evt.getHandle<artdaq::Fragments>(itag);

  if(cont_frags){
    for(auto const& cont : *cont_frags){
      artdaq::ContainerFragment contf(cont);                                           
      for(size_t f=0;f<contf.block_count(); ++f){
        artdaq::Fragment frag=*contf.at(f).get();
        ptbcontcounter++;
        analyze_ptb_fragment(frag, eventcounter);
      }
    }
  } else {
    ptb_cont_frag = false; 
  }


  // Get the fragment information

  // default
  //auto fragmentHandles = evt.getMany<artdaq::Fragments>();

  // use function from PMT decoder
  auto fragmentHandles = readHandles( evt ); 

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
                     //if(!ptb_cont_frag) {
                       analyze_ptb_fragment(frag, eventcounter); 
                       ptbfragcounter++;
                     //}
                  break;

                  // Fragment from TDC - CRT t1 reset, BES, flash and event triggers
                  case (sbndaq::detail::FragmentType::TDCTIMESTAMP) : 
                  
                     analyze_tdc_fragment(frag); 
                     
                  break;

		
                   
            }
        
        }

    }//end loop over handle
  
  }//end loop over all handles

  hlt_counts[eventcounter][0] = evt.event();
  llt_counts[eventcounter][0] = evt.event();
  eventcounter++;
  ptbcontcounter=0;
  ptbfragcounter=0;
  ptb_cont_frag = true;

  analyze_event_metrics();
  analyze_trigger_rates();

  resetdatavectors();
  reseteventdatavectors();
  eventcounter = 0;
  
  sbndaq::SendEventMeta("snapshot:waveform:MSUM:3" , evt);
} // analyze

  

DEFINE_ART_MODULE(sbndaq::PTBdqm)
