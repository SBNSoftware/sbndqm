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
      int hlt_id;
      int llt_id;

      void analyze_caen_fragment(artdaq::Fragment frag);
      void analyze_ptb_fragment(artdaq::Fragment frag, int eventcounter);
      void analyze_tdc_fragment(artdaq::Fragment frag);
     // void printCPUUsage();
      void analyze_tdc_ptb();
      void resetdatavectors();
      static bool sortcol( const std::vector<uint64_t>& v1, const std::vector<uint64_t>& v2 );	
      uint32_t nChannels;

      std::vector<uint16_t>  fTicksVec;
      std::vector< std::vector<uint16_t> > fWvfmsVec;

//      std::vector<uint16_t>  events;

      //std::vector<uint64_t> llt_trigger;
      //std::vector<uint64_t> llt_ts;
      std::vector<uint64_t> llt_allbes_ts;
      //std::vector<uint64_t> hlt_trigger;
      //std::vector<uint64_t> hlt_ts;

      std::vector<uint64_t> ftdc_t1_utc;
      std::vector<uint64_t> ftdc_bes_utc;
      std::vector<uint64_t> ftdc_ch2_utc;
      std::vector<uint64_t> ftdc_flash_utc;
      std::vector<uint64_t> ftdc_event_utc;

      std::vector< std::vector<uint64_t> > llt_type_ts;
      std::vector< std::vector<uint64_t> > hlt_type_ts;
      std::vector< std::vector<uint64_t> > hlt_counts;
      std::vector< std::vector<uint64_t> > llt_counts;

      std::vector<uint64_t> flash_trigger_ts;
      std::vector<uint64_t> event_trigger_ts;
      std::vector<uint64_t> hlt_beam_ts;
      std::vector<uint64_t> hlt_offbeam_ts;
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
  //fEventBlock     = pset.get<int>("EventBlock",500);
  fBBES           = pset.get<int>("BeamBESGate",30);
  fOBBES          = pset.get<int>("OffBeamBESGate",26);
  fLight          = pset.get<int>("LightGate",20);
  fTDCT1          = pset.get<int>("TDCT1",0);
  fTDCBES         = pset.get<int>("TDCBES",1);
  //fTDCBES         = pset.get<int>("TDCBES",4);
  fTDCFlash       = pset.get<int>("TDCFlash",3);
  fTDCEvent       = pset.get<int>("TDCEvent",4);
  //fTDCEvent       = pset.get<int>("TDCEvent",1);

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

std::vector<art::Handle<artdaq::Fragments>> sbndaq::PTBdqm::readHandles( art::Event const & event ) const{
  std::vector<art::Handle<artdaq::Fragments>> handles;
  std::vector<std::string>  m_input_tags = { "CAENV1730", "ContainerCAENV1730", "ContainerPTB", "PTB", "ContainerTDCTIMESTAMP", "TDCTIMESTAMP"};
  for( std::string const& input_tag : m_input_tags ) {
    art::Handle<artdaq::Fragments> thisHandle;
    event.getByLabel("daq", input_tag, thisHandle);
    //std::cout << "got " << input_tag << " handles" << std::endl;
    if( !thisHandle.isValid() || thisHandle->empty() ) continue;
    handles.push_back( thisHandle );
  }
  return handles;
}

//------------------------------------------------------------------------------------------------------------------
/*
void sbndaq::PTBdqm::printCPUUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    std::cout << "User CPU time: " << usage.ru_utime.tv_sec << " seconds\n";
    std::cout << "System CPU time: " << usage.ru_stime.tv_sec << " seconds\n";
}*/
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
      // Checking if waveforms are printed correctly 
      // for(size_t g=0; g<fTicksVec.size(); g++){
      // std::cout << fTicksVec[g] << std::endl;
      // }


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

      // Loop through all the PTB words in the fragment
      //std::cout << "fLight = " << fLight <<std::endl;
      for ( size_t i = 0; i < ptb_fragment.NWords(); i++ ) {
    
         switch ( ptb_fragment.Word(i)->word_type ) {
    
            case 0x1 : // LL Trigger
            {
               uint64_t trigger_mask = ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF;
               
	       /*for (size_t bit = 0; bit < 64; bit++) { // Check each bit position
		   if (trigger_mask & (1ULL << bit)) { // If the bit is set
		       size_t lt_id = bit;
                       int llt_id = static_cast<int>(lt_id);

		       if (llt_id == fBBES || llt_id == fOBBES) {
			   llt_allbes_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20);
		       }
		       llt_type_ts[llt_id].emplace_back(ptb_fragment.TimeStamp(i) * 20);
		       ++llt_counts[eventcounter][llt_id+1]; 
		       //if(llt_id >= 27) std::cout << "LLT ID = " << llt_id << " timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;
                   }*/
               //option 2:
               //gave multiple triggers of same type in the same tick
	       while (trigger_mask) {
		   size_t lt_id = __builtin_ctzll(trigger_mask); // Get the least significant 1-bit index
		   trigger_mask &= (trigger_mask - 1); // Turn off the least significant 1-bit
                   int llt_id = static_cast<int>(lt_id);
                   //std::cout << "LLT ID = " << llt_id <<std::endl;
		   if (llt_id == fBBES || llt_id == fOBBES) {
		       llt_allbes_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20);
		   }
		   llt_type_ts[llt_id].emplace_back(ptb_fragment.TimeStamp(i) * 20);
		   ++llt_counts[eventcounter][llt_id+1]; 

		   //std::cout << "LLT ID = " << llt_id << std::endl;
	       }
               break;
            }
               /*
               llt_id = round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2));
               //llt_trigger.emplace_back(round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2)) ); 
               //llt_trigger.emplace_back(llt_id); 
               //llt_ts.emplace_back( ptb_fragment.TimeStamp(i) * 20 );
               if(llt_id == fBBES || llt_id == fOBBES){
                  //std::cout << "BES LLT = " << llt_id << ", with timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;
                  llt_allbes_ts.emplace_back( ptb_fragment.TimeStamp(i) * 20 );
               }
               llt_type_ts[llt_id].emplace_back( ptb_fragment.TimeStamp(i) * 20 );
               //std::cout << "LLT ID = " << llt_id <<std::endl;
               //if(llt_id == fLight) std::cout<< "Let there be light: LLT ID = " << fLight <<std::endl;
            break;
            */
      
            case 0x2 : // HL Trigger
            {
               uint64_t trigger_mask = ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF;
               /*std::cout << "Method 2 original" <<std::endl;
               hlt_id = round(log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2));
	       if(hlt_id >= 0 && hlt_id <= 19 ){
		  std::cout<<"Event trigger found " << hlt_id << std::endl;
	       }
               std::cout << "Method 3 for" <<std::endl;
	       for (size_t bit = 0; bit < 64; bit++) { // Check each bit position
		   if (trigger_mask & (1ULL << bit)) { // If the bit is set
		       size_t ht_id = bit;
                       int hlt_id = static_cast<int>(ht_id);
		       if(hlt_id >= 0 && hlt_id <= 19 ){
		 	 std::cout<<"Event trigger found " << hlt_id << std::endl;
		       }
                   }
               }
               std::cout << "Method 1 while" <<std::endl;*/
               while (trigger_mask) {
                   size_t ht_id = __builtin_ctzll(trigger_mask); // Get the least significant 1-bit index
                   trigger_mask &= (trigger_mask - 1); // Turn off the least significant 1-bit
                   int hlt_id = static_cast<int>(ht_id);
                   if(hlt_id < 32){
		      //std::cout << "HLT ID = " << hlt_id <<std::endl;
		      hlt_type_ts[hlt_id].emplace_back(ptb_fragment.TimeStamp(i) * 20);
		      if(hlt_id >= 22 && hlt_id <= 30 ){
			 flash_trigger_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
		      }
		      if(hlt_id == 20 || hlt_id == 21){
			 crt_t1reset_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
		      }
		      if(hlt_id >= 0 && hlt_id <= 19 ){
			 event_trigger_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
			 //std::cout<<"Event trigger found " << hlt_id << std::endl;
		      }
		      if(hlt_id == 1 || hlt_id == 2 ){
			 hlt_beam_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
		      }
		      if(hlt_id == 3 || hlt_id == 4 ){
			 hlt_offbeam_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
		      }
		      ++hlt_counts[eventcounter][hlt_id+1];
                   }
               }
               break;
             }
               //std::cout<< "log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2) = " << log(ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF)/log(2) << " id = " << hlt_id << std::endl;
               //hlt_trigger.emplace_back( hlt_id );
               // Assuming HLTs 22 and above are flash triggers.
               /*if(hlt_id >= 22 && hlt_id <= 30 ){
                  flash_trigger_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );
                  //std::cout << "Flash found, HLT = " << hlt_id << ", with timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;
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
                  //std::cout << "T1 Reset found, HLT = " << hlt_id << ", with timestamp = " << ptb_fragment.TimeStamp(i) * 20 << std::endl;
               }
               //if(hlt_id == 20){crt_t1reset_b_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               //if(hlt_id == 21){crt_t1reset_ob_ts.emplace_back(ptb_fragment.TimeStamp(i) * 20 );}
               //hlt_ts.emplace_back( ptb_fragment.TimeStamp(i) * 20 );
               hlt_type_ts[hlt_id].emplace_back( ptb_fragment.TimeStamp(i) * 20 );
               //if(hlt_id == 29) std::cout << "HLT 29 TS = " << ptb_fragment.TimeStamp(i) * 20 <<std::endl;
               //std::cout << "event count index = " << eventcounter << std::endl;
               ++hlt_counts[eventcounter][hlt_id+1]; 

            break;*/
         }
      }

}

void sbndaq::PTBdqm::analyze_tdc_fragment(artdaq::Fragment frag) {

      TDCTimestampFragment tsfrag = TDCTimestampFragment(frag);
      const TDCTimestamp* ts = tsfrag.getTDCTimestamp();
      //std::cout << "Looking at the TDC"<<std::endl;
      // CRT t1 reset 
      if (ts->vals.channel==fTDCT1) {
         ftdc_t1_utc.emplace_back(ts->timestamp_ns());
         //std::cout << "TDC T1 with timestamp = " << ts->timestamp_ns() << std::endl;
      }

      // BES 
      if (ts->vals.channel==fTDCBES) {
         ftdc_bes_utc.emplace_back(ts->timestamp_ns());
         //std::cout << "TDC BES with timestamp = " << ts->timestamp_ns() << std::endl;
      }

      // RWM
      //  if (ts->vals.channel==2) {ftdc_ch2_utc.emplace_back(ts->timestamp_ns());}

      // PTB flash trigger
      if (ts->vals.channel==fTDCFlash) {
         ftdc_flash_utc.emplace_back(ts->timestamp_ns());
         //std::cout << "TDC Flash with timestamp = " << ts->timestamp_ns() << std::endl;
      }

      // PTB event trigger
      if (ts->vals.channel==fTDCEvent) {
         ftdc_event_utc.emplace_back(ts->timestamp_ns());
         //std::cout << "TDC Event with timestamp = " << ts->timestamp_ns() << std::endl;
      }

}

void sbndaq::PTBdqm::analyze_tdc_ptb() {

      //sort(events.begin(), events.end());
      sort(hlt_counts.begin(),hlt_counts.end(),sortcol);
      sort(llt_counts.begin(),llt_counts.end(),sortcol);
/**************************************************************************************************************************************/
/************************************** TRIGER RATES **********************************************************************************/
/**************************************************************************************************************************************/
      //llt_type_ts.resize(32);
      //hlt_type_ts.resize(32);
      //hlt_type_ts_test.resize(32);
      
      // Calculate LLT and HLT rates. Loop over all possible IDs for Low- and High-Level triggers and get the correspondent timestamps
      // LLT rate     
      /*for (size_t i = 0; i < llt_counts.size(); i++)
	 {
	     for (size_t j = 0; j < llt_counts[i].size(); j++)
	     {
		 std::cout << llt_counts[i][j] << " ";
	     }    
	     std::cout << std::endl;
	 }
	     std::cout << std::endl;*/
      /*for (size_t i = 0; i < llt_type_ts.size(); i++)
	 {
	     for (size_t j = 0; j < llt_type_ts[i].size(); j++)
	     {
		 std::cout << llt_type_ts[i][j] << " ";
	     }    
	     std::cout << std::endl;
	 }*/
      for(size_t q=0; q<32; q++){
         std::string lt_id = std::to_string(q);
         if (llt_type_ts[q].size() == 0) {
              sbndaq::sendMetric("LLT_ID", lt_id, "LLT_periodicity", 0, fReportingLevel, artdaq::MetricMode::Average);
              continue;
         }  
         //std::cout << "Found LLT " << lt_id <<std::endl;
         //std::cout << "Found LLT " << lt_id << " entries = " << llt_type_ts[q].size() << std::endl;

         sort(llt_type_ts[q].begin(), llt_type_ts[q].end());

         /*if(q<1){
	    for (size_t i = 0; i < llt_type_ts.size(); i++) {
		for (size_t j = 0; j < llt_type_ts[i].size(); j++){
		   //if(llt_type_ts[i][j] < 1728300000000000000) std::cout << llt_type_ts[i][j] << " ";
		   std::cout << llt_type_ts[i][j] << " ";
		}    
		std::cout << std::endl;
	    }
         }*/
         //std::cout << "Sorted timestamps for LLT" << lt_id <<std::endl;
	 std::vector<size_t> skip_index;
	 for(size_t i = 0; i < llt_counts.size()-1; i++){
	    if(llt_counts[i+1][0]-llt_counts[i][0] != 1){ //comparing event numbers
               //std::cout<<"last event missed = "<<llt_counts[i+1][0] - 1 <<std::endl;
	       size_t index_to_skip = 0;
	       for(size_t j = 0; j < i+1; j++){
                  //std::cout << "llt count id " << q << " count " << llt_counts[j][q+1]<<std::endl;
		  index_to_skip = index_to_skip + llt_counts[j][q+1];
	       }
	       if(index_to_skip != 0){
                  skip_index.push_back(index_to_skip);
                  //std::cout<< "index_to_skip (s+1) = " <<index_to_skip<<std::endl;
               }
	    }
	 }
         size_t curr_skip = 0;
         for(size_t s=0; s<llt_type_ts[q].size()-1; s++){
            if(skip_index.size()>0){
	       if(s+1 == skip_index[curr_skip]){
		  ++curr_skip;
		  continue;
               }
            }
            uint64_t diff;
            double diff_s;
            if (llt_type_ts[q][s+1] > llt_type_ts[q][s]) {
               diff = (llt_type_ts[q][s+1]-llt_type_ts[q][s]);
               diff_s = diff*pow(10,-9);
            } else {
               diff = (llt_type_ts[q][s]-llt_type_ts[q][s+1]);
               diff_s = diff*pow(10,-9)*-1;
               //std::cout << "somethings up with llt periodicity: llt_id = " << q << ", s+1 = " << llt_type_ts[q][s+1] << ", s = " << llt_type_ts[q][s] << " diff = " << diff_s << std::endl;
               continue;
            }
            sbndaq::sendMetric("LLT_ID", lt_id, "LLT_periodicity", diff_s, fReportingLevel, artdaq::MetricMode::Average);
            //if(s<10) std::cout << "LLT " << lt_id << " s+1 = " << s+1 << " s = " << s << " periocity  = " << llt_type_ts[q][s+1] << "- " <<  llt_type_ts[q][s] << " = " << diff_s << " s" << std::endl;
            //if(q == 4) std::cout << "LLT " << lt_id << " periocity  = " << llt_type_ts[q][s+1] << "- " <<  llt_type_ts[q][s] << " = " << diff_s << " s" << std::endl;
         }
      }

      // HLT rate
      /*for (size_t i = 0; i < hlt_counts.size(); i++)
	 {
	     for (size_t j = 0; j < hlt_counts[i].size(); j++)
	     {
		 std::cout << hlt_counts[i][j] << " ";
	     }    
	     std::cout << std::endl;
	 }*/
      for(size_t q=0; q<32; q++){
         std::string ht_id = std::to_string(q);
         if(hlt_type_ts[q].size() == 0){
            sbndaq::sendMetric("HLT_ID", ht_id, "HLT_periodicity", 0, fReportingLevel, artdaq::MetricMode::Average);
            continue;
         }
         //std::cout << "Found HLT " << ht_id << " entries = " << hlt_type_ts[q].size() << std::endl;
         sort(hlt_type_ts[q].begin(), hlt_type_ts[q].end());
         //std::cout << "Sorted timestamps for HLT" << ht_id <<std::endl;
	 std::vector<size_t> skip_index;
	 for(size_t i = 0; i < hlt_counts.size()-1; i++){
	    if(hlt_counts[i+1][0]-hlt_counts[i][0] != 1){ //comparing event numbers
               //std::cout<<"last event missed = "<<hlt_counts[i+1][0] - 1 <<std::endl;
	       size_t index_to_skip = 0;
	       for(size_t j = 0; j < i+1; j++){
                  //std::cout << "hlt count id " << q << " count " << hlt_counts[j][q+1]<<std::endl;
		  index_to_skip = index_to_skip + hlt_counts[j][q+1];
	       }
	       if(index_to_skip != 0){
                  skip_index.push_back(index_to_skip);
                  //std::cout<< "index_to_skip (s+1) = " <<index_to_skip<<std::endl;
               }
	    }
	 }
         size_t curr_skip = 0;
         for(size_t s=0; s<hlt_type_ts[q].size()-1; s++){
            if(skip_index.size()>0){
	       if(s+1 == skip_index[curr_skip]){
		  ++curr_skip;
		  continue;
               }
            }
            uint64_t diff;
            double diff_s;
            if (hlt_type_ts[q][s+1] > hlt_type_ts[q][s]) {
               diff = (hlt_type_ts[q][s+1]-hlt_type_ts[q][s]);
               diff_s = diff*pow(10,-9);
            } else {
               diff = (hlt_type_ts[q][s]-hlt_type_ts[q][s+1]);
               diff_s = diff*pow(10,-9)*-1;
               //std::cout << "somethings up with hlt periodicity: hlt_id = " << q << ", s+1 = " << hlt_type_ts[q][s+1] << ", s = " << hlt_type_ts[q][s] << " diff = " << diff_s << std::endl;
               continue;
            }
            sbndaq::sendMetric("HLT_ID", ht_id, "HLT_periodicity", diff_s, fReportingLevel, artdaq::MetricMode::Average);
            /*if(diff_s > 1 || diff_s<=0) {
               std::cout << std::endl;
               std::cout << "Odd periodicity: HLT " << ht_id << " = " << diff_s << " s" << std::endl;
               std::cout << "All HLT " << ht_id << " TSs : " << std::endl;
               for(size_t i = 0; i < hlt_type_ts[q].size(); i++){
                  std::cout << hlt_type_ts[q][i] << "   ";
               }
               std::cout << std::endl;
            }*/
         }
      }


/**************************************************************************************************************************************/
/************************************** PTB BES - light timestamp distribution ********************************************************/
/**************************************************************************************************************************************/

      //distribution of light triggers around BES (start of beam acceptance)
      size_t init_l = 0;
      //std::cout<<"llt_type_ts[fBBES].size() = " <<llt_type_ts[fBBES].size()<<std::endl;
      //std::cout<<"llt_type_ts[fOBBES].size() = " <<llt_type_ts[fOBBES].size()<<std::endl;
      //std::cout<<"llt_type_ts[fLight].size() = " <<llt_type_ts[fLight].size()<<std::endl;
      for(size_t k=0; k<llt_type_ts[fBBES].size(); k++){
          bool inrange = false;
          for(size_t l = init_l; l < llt_type_ts[fLight].size(); l++){
              uint64_t diff;
              double diff_sign;
              if (llt_type_ts[fBBES][k] > llt_type_ts[fLight][l]){
                 diff = llt_type_ts[fBBES][k]-llt_type_ts[fLight][l];
                 diff_sign = -0.001 * diff; //want -ve when ligth before bes, us
              } else {
                 diff = llt_type_ts[fLight][l] - llt_type_ts[fBBES][k]; //want +ve when ligth after bes
                 diff_sign = diff*0.001; //us
              }
              if(diff <= 10000){ // 10us
                  //std::cout << " beam light - bes = " << llt_type_ts[fLight][l] << " - " << llt_type_ts[fBBES][k] << " = " << diff_sign << std::endl;
                  inrange = true;
                  sbndaq::sendMetric("BEAM_LIGHT_DIFF","0","BEAM_LIGHT", diff_sign, fReportingLevel, artdaq::MetricMode::Average);
              }else if(inrange){
                  init_l = l;
                  break;
              }
          }
      }

      //distribution of light triggers around offbeam BES (start of off beam acceptance)
      init_l = 0;
      for(size_t k=0; k<llt_type_ts[fOBBES].size(); k++){
          //std::cout << "k = " << k << std::endl;
          bool inrange = false;
          for(size_t l = init_l; l < llt_type_ts[fLight].size(); l++){
              uint64_t diff;
              double diff_sign;
              if (llt_type_ts[fOBBES][k] > llt_type_ts[fLight][l]){
                 diff = llt_type_ts[fOBBES][k]-llt_type_ts[fLight][l];
                 diff_sign = -0.001 * diff; //want -ve when ligth before bes
              } else {
                 diff = llt_type_ts[fLight][l] - llt_type_ts[fOBBES][k]; //want +ve when ligth after bes
                 diff_sign = diff * 0.001;
              }
              if(diff <= 10000){ //10us
                  //std::cout << "off beam light - bes = " << llt_type_ts[fLight][l] << " - " << llt_type_ts[fOBBES][k] << " = " << diff_sign << " us"  << std::endl;
                  inrange = true;
                  sbndaq::sendMetric("BEAM_LIGHT_DIFF","0","OFFBEAM_LIGHT", diff_sign, fReportingLevel, artdaq::MetricMode::Average);
              }else if(inrange){
                  init_l = l;
                  break;
              }
          }
      }

      //distribution of light triggers around offbeam BES (start of off beam acceptance)
      /*init_l = 0;
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
      }*/

/**************************************************************************************************************************************/
/************************************** PTB (off) beam Event - PTB CRT (off) beam Reset timestamps ************************************/
/**************************************************************************************************************************************/

      // Beam HLTs - Beam T1 Reset
      //std::cout << "size beam HLTs: " << hlt_beam_ts.size() << ", size beam T1 resets " << hlt_type_ts[20].size() << std::endl;
      if(hlt_beam_ts.size() == hlt_type_ts[20].size()) {
	  /*std::sort(hlt_beam_ts.begin(), hlt_beam_ts.end());
	  std::cout << "Beam HLT TSs" <<std::endl;
 	  for(size_t i=0;i<hlt_beam_ts.size(); i++){
	      std::cout << hlt_beam_ts[i]<< " ";
	  }    
	  std::cout << std::endl;
	  std::cout << "CRT T1 TSs" <<std::endl;
	  for(size_t i=0;i<hlt_type_ts[20].size(); i++){
	      std::cout << hlt_type_ts[20][i]<< " ";
	  }    
	  std::cout << std::endl;*/
          for(size_t k=0; k<hlt_type_ts[20].size(); k++){
              if (hlt_beam_ts[k] > hlt_type_ts[20][k]){
		  uint64_t diff = hlt_beam_ts[k] - hlt_type_ts[20][k];
		  double diff_us = diff * 0.001;
		  sbndaq::sendMetric("BEAM_CRT_DIFF","0","BEAM_HLT_T1RESET", diff_us, fReportingLevel, artdaq::MetricMode::Average);
                  //std::cout << "BEAM HLT - BEAM T1 RESET: " << diff_us << " microseconds" << std::endl;
              }//else std::cout<< "Issue with beam crt t1 diff: hlt_beam_ts[k] = " << hlt_beam_ts[k] << " hlt_type_ts[20][k] = "<< hlt_type_ts[20][k] << std::endl;
          }
      }else {
          sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_BEAM_HLT", hlt_beam_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_BEAM_HLT: " <<  hlt_type_ts[1].size()+hlt_type_ts[2].size() << std::endl;

          sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_BEAM_T1RESET", hlt_type_ts[20].size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_BEAM_T1RESET: " << hlt_type_ts[20].size() << std::endl;
      }

      // Off Beam HLTs - Off Beam T1 Reset
      //std::cout << "size off beam HLTs: " << hlt_offbeam_ts.size() << ", size off beam T1 resets " << hlt_type_ts[21].size() << std::endl;
      if(hlt_offbeam_ts.size() == hlt_type_ts[21].size()) {
          //std::cout << "hlt_type_ts[3].size()+hlt_type_ts[4].size() == hlt_type_ts[21].size() == " << hlt_type_ts[3].size()+hlt_type_ts[4].size() << std::endl;
          std::sort(hlt_offbeam_ts.begin(), hlt_offbeam_ts.end());
	  /*std::cout << "Off-Beam HLT TSs" <<std::endl;
	  for(size_t i=0;i<hlt_offbeam_ts.size(); i++){
	      std::cout << hlt_offbeam_ts[i]<< " ";
	  }    
	  std::cout << std::endl;
	  std::cout << "CRT T1 TSs" <<std::endl;
	  for(size_t i=0;i<hlt_type_ts[21].size(); i++){
	      std::cout << hlt_type_ts[21][i]<< " ";
	  }    
	  std::cout << std::endl;*/
          for(size_t k=0; k<hlt_type_ts[21].size(); k++){
              if(hlt_offbeam_ts[k] > hlt_type_ts[21][k]){
                  uint64_t diff = hlt_offbeam_ts[k] - hlt_type_ts[21][k];
                  double diff_us = diff*0.001;
                  sbndaq::sendMetric("BEAM_CRT_DIFF","0","OFFBEAM_HLT_T1RESET", diff_us, fReportingLevel, artdaq::MetricMode::Average);
                  //std::cout << "OFFBEAM HLT - OFFBEAM T1 RESET: " << diff_us << " microseconds" << std::endl;
              }//else std::cout<< "Issue with offbeam crt t1 diff: hlt_offbeam_ts[k] = " << hlt_offbeam_ts[k] << " hlt_type_ts[21][k] = "<< hlt_type_ts[21][k] << std::endl;
          }
      }else {
          sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_OFFBEAM_HLT", hlt_offbeam_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_OFFBEAM_HLT: " << hlt_type_ts[3].size()+hlt_type_ts[4].size() << std::endl;

          sbndaq::sendMetric("BEAM_CRT_DIFF","0","NUMBER_OFFBEAM_T1RESET", hlt_type_ts[21].size(), fReportingLevel, artdaq::MetricMode::Average);
          //std::cout << "NUMBER_OFFBEAM_T1RESET: " << hlt_type_ts[21].size() << std::endl;
      }

/**************************************************************************************************************************************/
/************************************** PTB - TDC timestamps **************************************************************************/
/**************************************************************************************************************************************/

      // Calculate PTB - TDC timestamp differences
      sort(ftdc_t1_utc.begin(), ftdc_t1_utc.end());
      sort(ftdc_bes_utc.begin(), ftdc_bes_utc.end());
      sort(ftdc_flash_utc.begin(), ftdc_flash_utc.end());
      sort(ftdc_event_utc.begin(), ftdc_event_utc.end());
      sort(llt_allbes_ts.begin(), llt_allbes_ts.end());
      sort(flash_trigger_ts.begin(), flash_trigger_ts.end());
      sort(event_trigger_ts.begin(), event_trigger_ts.end());
      sort(crt_t1reset_ts.begin(), crt_t1reset_ts.end());
      // Flash triggers and TDC channel 4.
      /*if(ftdc_flash_utc.size() == flash_trigger_ts.size()) {
         for(size_t k=0; k<flash_trigger_ts.size(); k++){
            uint64_t diff;
            double diff_us;
            if(ftdc_flash_utc[k] > flash_trigger_ts[k]){
               diff = ftdc_flash_utc[k] - flash_trigger_ts[k];
               diff_us = diff*0.001;
            }else diff_us = 999999999999;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC4_HLTFLASH", diff_us, fReportingLevel, artdaq::MetricMode::Average);
         }
      }else {*/
         bool match_found = false;
         size_t a = 0;
         size_t i_diff = 0;
         for(; a < flash_trigger_ts.size(); a++){
            for(size_t b = 0; b < ftdc_flash_utc.size(); b++){
               uint64_t diff = ftdc_flash_utc[b] - flash_trigger_ts[a];
               double diff_us = diff*0.001;
               if(diff_us < 1000){
                  i_diff = a - b;
                  match_found = true;
                  break;
               }
            }
            if(match_found) break;
         }
         if(match_found){
            //size_t shift_tdc = 0;
            //size_t shift_ptb = 0;
            /*std::cout << "FLASH TSs" <<std::endl;
            for(size_t i=0;i<flash_trigger_ts.size(); i++){
                std::cout << flash_trigger_ts[i]<< " ";
            }    
            std::cout << std::endl;
            std::cout << "TDC TSs" <<std::endl;
            for(size_t i=0;i<ftdc_flash_utc.size(); i++){
                std::cout << ftdc_flash_utc[i]<< " ";
            }    
            std::cout << std::endl;
*/
            for(size_t i = a; i < flash_trigger_ts.size(); i++){
               uint64_t diff;
               double diff_us;
               if(flash_trigger_ts[i] < ftdc_flash_utc[i-i_diff]){
                  if(ftdc_flash_utc[i-i_diff] <= 0 || flash_trigger_ts[i] <= 0 ) continue;
	          diff = ftdc_flash_utc[i-i_diff] - flash_trigger_ts[i];
                  diff_us = diff*0.001; //want positive when flash ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
                                        //could leave it with the unsigned int error not copeing with negatvies adn going into underflow (very large numebr) to highlight this
               }else {
	          diff = flash_trigger_ts[i] - ftdc_flash_utc[i-i_diff];
                  diff_us = diff*-0.001; //want positive when flash ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
               }
            /*for(size_t i = a; i < flash_trigger_ts.size(); i++){
               if(i-i_diff+shift_tdc > ftdc_flash_utc.size()) break;
               uint64_t diff;
               double diff_us;
               if(flash_trigger_ts[i+shift_ptb] < ftdc_flash_utc[i-i_diff+shift_tdc]){
	          diff = ftdc_flash_utc[i-i_diff+shift_tdc] - flash_trigger_ts[i+shift_ptb];
                  diff_us = diff*0.001; //want positive when flash ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
                                        //could leave it with the unsigned int error not copeing with negatvies adn going into underflow (very large numebr) to highlight this
               }else {
	          diff = flash_trigger_ts[i+shift_ptb] - ftdc_flash_utc[i-i_diff+shift_tdc];
                  diff_us = diff*-0.001; //want positive when flash ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
               }
               if(diff_us > 1 || diff_us < 0){
		  uint64_t diff_test = ftdc_flash_utc[i-i_diff+shift_tdc+1] - flash_trigger_ts[i+shift_ptb];
		  if (diff_test < 1000){
		     shift_tdc = shift_tdc + 1;
		     diff_us = diff_test*0.001;
		  }else {
		     diff_test = ftdc_flash_utc[i-i_diff+shift_tdc] - flash_trigger_ts[i+shift_ptb+1];
		     if (diff_test < 1000){
			shift_ptb = shift_ptb + 1;
			diff_us = diff_test*0.001;
		     } else {
		        diff_test = ftdc_flash_utc[i-i_diff+shift_tdc+2] - flash_trigger_ts[i+shift_ptb];
		        if (diff_test < 1000){
		           shift_tdc = shift_tdc + 2;
		           diff_us = diff_test*0.001;
			}else {
			   diff_test = ftdc_flash_utc[i-i_diff+shift_tdc] - flash_trigger_ts[i+shift_ptb+2];
			   if (diff_test < 1000){
			      shift_ptb = shift_ptb + 2;
			      diff_us = diff_test*0.001;
			   } else {
			      diff_test = ftdc_flash_utc[i-i_diff+shift_tdc+2] - flash_trigger_ts[i+shift_ptb+1];
			      if (diff_test < 1000){
				 shift_tdc = shift_tdc + 2;
				 shift_ptb = shift_ptb + 1;
				 diff_us = diff_test*0.001;
			      }else {
				 diff_test = ftdc_flash_utc[i-i_diff+shift_tdc+1] - flash_trigger_ts[i+shift_ptb+2];
				 if (diff_test < 1000){
				    shift_ptb = shift_ptb + 2;
				    shift_tdc = shift_tdc + 1;
				    diff_us = diff_test*0.001;
				 } else {
				    diff_us = 999999999999; //big number so alarms
				 }
                              }
                           }
                        }
                     }
		  }
               }*/
	       //if(diff_us != 0.267) std::cout << "TDC4_HLTFLASH DIFF (excl 0.267) = " << diff_us << std::endl;
	       //std::cout << "TDC4_HLTFLASH DIFF = " << diff_us << std::endl;
	       //std::cout << "PTB_TDC_DIFF: i = " << i << ", i-i_diff = " << i-i_diff << ", TDC4_HLTFLASH = " << diff_us << std::endl;
	       sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_HLT_FLASH", diff_us, fReportingLevel, artdaq::MetricMode::Average);
            }
         }//else{
            sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_FLASH", ftdc_flash_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
	    //std::cout << "#TDC Flash = " << ftdc_flash_utc.size() << std::endl;
	    //std::cout << "#PTB Flash = " << flash_trigger_ts.size() << std::endl;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_FLASH", flash_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
         //}

      // Event triggers and TDC channel 5.
      /*if(ftdc_event_utc.size() == event_trigger_ts.size()) {
         for(size_t k=0; k<event_trigger_ts.size(); k++){
            uint64_t diff;
            double diff_us;
            if(ftdc_event_utc[k] > event_trigger_ts[k]){
               diff = ftdc_event_utc[k] - event_trigger_ts[k];
               diff_us = diff*0.001;
            } else diff_us = 999999999999;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC5_HLTEVENT", diff_us, fReportingLevel, artdaq::MetricMode::Average);
         }
      }else {
         bool match_found = false;
         size_t a = 0;
         size_t i_diff = 0;*/
         match_found = false;
         a = 0;
         i_diff = 0;
         for(; a < event_trigger_ts.size(); a++){
            for(size_t b = 0; b < ftdc_event_utc.size(); b++){
               uint64_t diff = ftdc_event_utc[b] - event_trigger_ts[a];
               double diff_us = diff*0.001;
               if(diff_us < 1){
               //if(diff_us < 1200000){
                  i_diff = a - b;
                  match_found = true;
                  //std::cout << "Starting at i+shift_ptb = " << a << " and i-i_diff+shift_tdc = " << b << std::endl;
                  break;
               }
            }
            if(match_found) break;
         }
	 /*std::cout << "EVENT TSs" <<std::endl;
	 for(size_t i=0;i<event_trigger_ts.size(); i++){
	     std::cout << event_trigger_ts[i]<< " ";
	 }    
	 std::cout << std::endl;
	 std::cout << "TDC TSs" <<std::endl;
	 for(size_t i=0;i<ftdc_event_utc.size(); i++){
	     std::cout << ftdc_event_utc[i]<< " ";
	 }    
	 std::cout << std::endl;*/
         if(match_found){
            size_t shift_tdc = 0;
            size_t shift_ptb = 0;
            /*std::cout << "EVENT TSs" <<std::endl;
            for(size_t i=0;i<event_trigger_ts.size(); i++){
                std::cout << event_trigger_ts[i]<< " ";
            }    
            std::cout << std::endl;
            std::cout << "TDC TSs" <<std::endl;
            for(size_t i=0;i<ftdc_event_utc.size(); i++){
                std::cout << ftdc_event_utc[i]<< " ";
            }    
            std::cout << std::endl;*/
            /*for(size_t i = a; i < event_trigger_ts.size(); i++){
               if(i-i_diff >= ftdc_event_utc.size()) break;
               uint64_t diff;
               double diff_us;
               if(event_trigger_ts[i] < ftdc_event_utc[i-i_diff]){
	          diff = ftdc_event_utc[i-i_diff] - event_trigger_ts[i];
                  diff_us = diff*0.001; //want positive when event ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
                                        //could leave it with the unsigned int error not copeing with negatvies adn going into underflow (very large numebr) to highlight this
               } else {
	          diff = event_trigger_ts[i] - ftdc_event_utc[i-i_diff];
                  diff_us = diff*-0.001; //want positive when event ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
               }*/
            for(size_t i = a; i < event_trigger_ts.size(); i++){
               /*double diff_event = 0;
               if(i > 0){
		  diff_event = event_trigger_ts[i+shift_ptb] - event_trigger_ts[i+shift_ptb-1];
                  //std::cout << "diff_event = " << event_trigger_ts[i+shift_ptb] << " - " << event_trigger_ts[i+shift_ptb-1] << " = " << diff_event << std::endl;
		  if(diff_event < 5000000){ //inhibit = 5ms = 5000000ns
		     //this event was inhibited so nothing send to tdc, skip to next event TS
		     shift_ptb = shift_ptb + 1;
                  }
               }
               std::cout << "diff_event = " << diff_event << ", shift_ptb  = " << shift_ptb << std::endl;*/
               if(i-i_diff+shift_tdc >= ftdc_event_utc.size()) break;
               if(i+shift_ptb >= event_trigger_ts.size()) break;
               double diff;
               double diff_us;
               if(event_trigger_ts[i+shift_ptb] < ftdc_event_utc[i-i_diff+shift_tdc]){
	          diff = ftdc_event_utc[i-i_diff+shift_tdc] - event_trigger_ts[i+shift_ptb];
                  diff_us = diff*0.001; //want positive when event ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
                                        //could leave it with the unsigned int error not copeing with negatvies adn going into underflow (very large numebr) to highlight this
               } else {
	          diff = event_trigger_ts[i+shift_ptb] - ftdc_event_utc[i-i_diff+shift_tdc];
                  diff_us = diff*-0.001; //want positive when event ts > tdc ts, ptb sends timestamp to tdc so if its bigger something has gone very wrong
               }/*
	       if(diff_us > 1 || diff_us < 0){
		  uint64_t diff_test = ftdc_event_utc[i-i_diff+shift_tdc+1] - event_trigger_ts[i+shift_ptb];
		  if (diff_test < 1000){
		     shift_tdc = shift_tdc + 1;
		     diff_us = diff_test*0.001;
		  }else {
		     diff_test = ftdc_event_utc[i-i_diff+shift_tdc] - event_trigger_ts[i+shift_ptb+1];
		     if (diff_test < 1000){
			shift_ptb = shift_ptb + 1;
			diff_us = diff_test*0.001;
		     }else {
			diff_us = 999999999999; //big number so alarms
                     }
                  }
               }*/
	       sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_HLT_EVENT", diff_us, fReportingLevel, artdaq::MetricMode::Average);
	       //std::cout << "PTB_TDC_DIFF: i+shift_ptb = " << i+shift_ptb << ", i-i+diff+shift_tdc = " << i-i_diff+shift_tdc << ", TDC5_HLTEVENT = " << diff_us << std::endl;
            }
         }//else {
            //std::cout << "PTB_TDC_DIFF: TDC5_HLTEVENT = NO MATCH"  << std::endl;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_EVENT", ftdc_event_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
	    //std::cout << "#TDC Event = " << ftdc_event_utc.size() << std::endl;
	    //std::cout << "#PTB Event = " << event_trigger_ts.size() << std::endl;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_EVENT", event_trigger_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
         //}
      //}


      // CRT t1 reset and TDC channel 1.
      /*if(ftdc_t1_utc.size() == crt_t1reset_ts.size()) {
         for(size_t k=0; k<crt_t1reset_ts.size(); k++){
	    uint64_t diff;
	    double diff_us;
	    if(crt_t1reset_ts[k] < ftdc_t1_utc[k]){
	       diff = ftdc_t1_utc[k] - crt_t1reset_ts[k];
	       diff_us = diff*0.001;
	    }else{
               diff_us = 999999999999; //big number so alarms
	    }
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC1_HLTT1", diff_us, fReportingLevel, artdaq::MetricMode::Average);
            std::cout << "PTB_TDC_DIFF: TDC1_HLTT1 = " << diff_us << std::endl;
         }
      }else {
      bool match_found = false;
      size_t a = 0;
      size_t i_diff = 0;*/
      match_found = false;
      a = 0;
      i_diff = 0;
      for(; a < crt_t1reset_ts.size(); a++){
	 for(size_t b = 0; b < ftdc_t1_utc.size(); b++){
	    uint64_t diff = ftdc_t1_utc[b] - crt_t1reset_ts[a];
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
	 for(size_t i = a; i < crt_t1reset_ts.size(); i++){
	    if(i-i_diff >= ftdc_t1_utc.size()) break;
	    if(i >= crt_t1reset_ts.size()) break;
	    //if(ftdc_t1_utc[i-i_diff] <= 0 || crt_t1reset_ts[i] <= 0) continue;
	    uint64_t diff;
	    double diff_us;
	    if(crt_t1reset_ts[i] < ftdc_t1_utc[i-i_diff]){
	       diff = ftdc_t1_utc[i-i_diff] - crt_t1reset_ts[i];
	       diff_us = diff*0.001;
	    } else{
	       diff_us = 999999999999; //big number so alarms
	    }
	    sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_HLT_T1", diff_us, fReportingLevel, artdaq::MetricMode::Average);
	    //std::cout << "PTB_TDC_DIFF: TDC1_HLTT1 = " << diff_us << std::endl;
	 }
      }//else {
	 sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_T1", ftdc_t1_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
	 //std::cout << "#TDC T1 = " << ftdc_t1_utc.size() << std::endl;
	 //std::cout << "#PTB T1 = " << crt_t1reset_ts.size() << std::endl;
	 sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_T1", crt_t1reset_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
      //}
      //}

      // TDC - LLT BES
      if(llt_allbes_ts.size() == ftdc_bes_utc.size()){
         for(size_t m=0; m<llt_allbes_ts.size(); m++){
	    uint64_t diff;
	    double diff_us;
	    if(llt_allbes_ts[m] < ftdc_bes_utc[m]){
	       diff = ftdc_bes_utc[m] - llt_allbes_ts[m];
	       diff_us = diff*0.001;
	    }else{
	       diff = llt_allbes_ts[m] - ftdc_bes_utc[m];
	       diff_us = diff*-0.001;
	    }
            sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_LLT_BES", diff_us, fReportingLevel, artdaq::MetricMode::Average);
            //std::cout << "PTB_TDC_DIFF: TDC2_LLTBES = " << diff_us << std::endl;
         }
      }else {
         bool match_found = false;
         size_t a = 0;
         size_t i_diff = 0;
         for(; a < llt_allbes_ts.size(); a++){
            for(size_t b = 0; b < ftdc_bes_utc.size(); b++){
               uint64_t diff = ftdc_bes_utc[b] - llt_allbes_ts[a];
               double diff_us = diff*0.001;
               if(diff_us < 1000){
                  i_diff = a - b;
                  match_found = true;
                  break;
               }
            }
            if(match_found) break;
         }
         if(match_found){
            for(size_t i = a; i < llt_allbes_ts.size(); i++){
               if(ftdc_bes_utc[i-i_diff] <= 0 || llt_allbes_ts[i] <= 0) continue;
               if(i-i_diff > ftdc_bes_utc.size()) break;
               uint64_t diff = ftdc_bes_utc[i-i_diff] - llt_allbes_ts[i];
               double diff_us = diff*0.001;
               sbndaq::sendMetric("PTB_TDC_DIFF","0","TDC_LLT_BES", diff_us, fReportingLevel, artdaq::MetricMode::Average);
               //std::cout << "PTB_TDC_DIFF: TDC2_LLTBES = " << diff_us << std::endl;
            }
         }//else {
            sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_TDC_BES", ftdc_bes_utc.size(), fReportingLevel, artdaq::MetricMode::Average);
	    //std::cout << "#TDC BES = " << ftdc_bes_utc.size() << std::endl;
	    //std::cout << "#PTB BES = " << llt_allbes_ts.size() << std::endl;
            sbndaq::sendMetric("PTB_TDC_DIFF","0","NUMBER_PTB_BES", llt_allbes_ts.size(), fReportingLevel, artdaq::MetricMode::Average);
         //}
      }

}

void::sbndaq::PTBdqm::resetdatavectors(){
  
  // Reset data vectors

  //events.clear();

  // PTB
  //llt_trigger.clear();
  //llt_ts.clear();
  llt_allbes_ts.clear();
  //hlt_trigger.clear();
  //hlt_ts.clear();

  llt_type_ts.clear();
  hlt_type_ts.clear();
  hlt_counts.clear();
  llt_counts.clear();

  flash_trigger_ts.clear();
  event_trigger_ts.clear();
  hlt_beam_ts.clear();
  hlt_offbeam_ts.clear();
  //event_trigger_b_ts.clear();
  //event_trigger_ob_ts.clear();
  crt_t1reset_ts.clear();
  //crt_t1reset_b_ts.clear();
  //crt_t1reset_ob_ts.clear();

  // TDC
  ftdc_t1_utc.clear();
  ftdc_bes_utc.clear();
  ftdc_ch2_utc.clear();
  ftdc_flash_utc.clear();
  ftdc_event_utc.clear();
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
 
  // -------DEBUGGING------

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
  
                  // Fragment from PTB - LLT and HLT production
                  case (sbndaq::detail::FragmentType::PTB) : 
                                                   
                     for (size_t ii = 0; ii < cont_frag.block_count(); ++ii){
                        
                        analyze_ptb_fragment(*cont_frag[ii].get(), eventcounter);
                       
                     
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
                  
                     analyze_ptb_fragment(frag, eventcounter); 
                     
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
  //events.push_back(evt.event());
  hlt_counts[eventcounter][0] = evt.event();
  llt_counts[eventcounter][0] = evt.event();
  //std::cout << "hlt_counts[eventcounter][0] = evt.event() = " << hlt_counts[eventcounter][0] << " = " << evt.event() << std::endl;
  // Set an event counter to controll data vector resets
  eventcounter++;

  // Impose data vector resetting after counting "fEventBlock" events
  if(eventcounter == fEventBlock){
      //printCPUUsage(); 
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
