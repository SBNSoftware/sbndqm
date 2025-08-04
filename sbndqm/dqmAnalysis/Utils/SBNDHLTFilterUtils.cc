#include "SBNDHLTFilterUtils.hh"

namespace sbndqm {
  namespace SBNDHLTFilterUtils{

  std::vector<uint64_t> GetAllHLTs(artdaq::ContainerFragment*  ptb_container_fragment){
    std::vector<uint64_t> triggers;  
  
    for (size_t f=0; f<ptb_container_fragment->block_count(); ++f){//loop over container of fragments
      artdaq::Fragment frag=*ptb_container_fragment->at(f).get();
      sbndaq::CTBFragment ptb_fragment(frag);
      //==============
      std::vector fragTriggers=GetHLT(ptb_fragment); //Not sure that there could really be multiple HLTs in a single fragment but just in case I'll make it vector
      triggers.insert(triggers.end(), fragTriggers.begin(), fragTriggers.end() );//append list of triggers in this fragment to all of the HLTs in the container
    }//end loop over fragments
  
    return triggers;
  }

  std::vector<uint64_t> GetHLT(sbndaq::CTBFragment ptb_fragment){
    std::vector<uint64_t> triggers;  
  
    for ( size_t i = 0; i < ptb_fragment.NWords(); i++ ) {//loop over words in fragment       
      //if  (ptb_fragment.Word(i)->IsHLT()==false) continue;  
      if  (ptb_fragment.Word(i)->word_type !=0x2 ) continue; //0x2 is the type for an HLT (0x1 for LLT) 
      //uint64_t hlttrigger=ptb_fragment.Trigger(i)->trigger_word & 0x1FFFFFFFFFFFFFFF;
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

  bool ApplyGateFilter(std::vector<uint64_t> triggers, std::vector<uint64_t> ftrigger_type, std::vector<uint64_t> fexcluded_triggers)//artdaq::ContainerFragment*  ptb_container_fragment)//(T trigfrag)
  {
  
    //std::cout << "ApplyGateFilter function start\n";
  
    std::string trigTypeList="{";//create list of the selected trigger types
    for (int trigger_type: ftrigger_type ){//get a string with the list of all the trigger types we're looking for 
      trigTypeList+=trigger_type+",";
    }
    trigTypeList+="}";
  
  
  
    if(triggers.size()==0){
      TLOG(TLVL_ERROR) << "This event has no HLT fragments or none with trigger numbers less than 20. It fails filter.";
      return false;
    }
  
    if(triggers.size()>1){
      TLOG(TLVL_INFO) << "This event has "<<triggers.size()<<" HLTs in it. Filter will pass if any are trigger type == "<<trigTypeList.c_str()<<".";
    }
  
    std::string trigstring="";
    bool passesFilter=false;
  
    for(int hlttrigger: triggers){//Loop over the HLTs found in the fragment
      if(fexcluded_triggers.size()>0){//Need to loop through all of the excluded trigger types to check for them for each present HLT 
        for(  int excluded_trigger : fexcluded_triggers){//loop over the list of triggers to exclude from the fcl
  	if (hlttrigger==excluded_trigger){
  	  //std::cout << "This Event contains an excluded trigger type " << hlttrigger << "==" << excluded_trigger<< " so fails the filter.\n";
  	  return false;
  	}
        }
      }//end loop over excluded triggers
      for( int trigger_type: ftrigger_type){//loop over the list of triggers to include from the fcl
        if( hlttrigger==trigger_type || trigger_type==-1){
  	//std::cout << "This Event has trigger type " << hlttrigger << "==" << trigstring.c_str() //trigger_type
  	//		 << "  and passes filter.\n";
  	passesFilter=true;
  	if(fexcluded_triggers.size()==0) break;//no need to keep looking through the triggers if none are excluded
        }
        trigstring+= std::to_string(hlttrigger)+", ";
      }
    }//end loop hlts 
  
    if (passesFilter) return true;
    
    //std::cout << "This Event has trigger type { " << trigstring.c_str()  << "} ==" 
    //		   << trigTypeList.c_str() << " and fails filter.\n";
    return false;
  
  }

  } //namespace SBNDHLTFilterUtils
} //namespace sbndqm
