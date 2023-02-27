////////////////////////////////////////////////////////////////////////
// Class:       CRTRawDecoder
// Plugin Type: producer (art v2_10_03)
// File:        CRTRawDecoder_module.cc
//
// Generated at Wed Jul  4 08:14:43 2018 by Andrew Olivier using cetskelgen
// from cetlib version v3_02_00.
////////////////////////////////////////////////////////////////////////
//AC: Changes made:
//Commented things out that seem unnecessary for compiling.
//
//
//Framework includes
//AC: Changed to match directories of BernCRTdqm_module.cc
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
//#include "art/Framework/Services/Optional/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
//AC: #include "messagefacility/MessageLogger/MessageLogger.h"
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
//LArSoft includes
//AC: #include "larcore/Geometry/Geometry.h"
//AC: #include "larcorealg/Geometry/GeometryCore.h"

//dune-artdaq includes
#include "artdaq-core/Data/ContainerFragment.hh"
//Log system
#include "TRACE/tracemf.h"
//dunepdlegacy includes
//AC: #include "dunepdlegacy/Overlays/CRTFragment.hh"
//AC: Moved location: /home/nfs/icarus/DQM_DevAreas/DQM_4April22/srcs/sbndqm/sbndqm/Decode/CRT/
#include "Fragments/CRTFragment.hh"
//#include "sbndaq-artdaq-core/Overlays/CRTFragment.hh"
//dunetpc includes
//#include "duneprototypes/Protodune/singlephase/CRT/data/CRTTrigger.h"
//AC: Moved location: /home/nfs/icarus/DQM_DevAreas/DQM_4April22/srcs/sbndqm/sbndqm/Decode/CRT/
#include "DATA/CRTTrigger.h"
//AC: #include "dunecore/Geometry/ProtoDUNESPCRTSorter.h"

//ROOT includes
#include "TGraph.h"

//c++ includes
#include <memory>

using namespace CRT;

//A CRTRawDecoder takes artdaq::Fragments made from Cosmic Ray Tagger input 
//and produces a CRT::Trigger for each time a CRT module triggered in this Event.  
namespace CRT
{
  class CRTRawDecoder : public art::EDProducer 
  {
    public:
      explicit CRTRawDecoder(fhicl::ParameterSet const & p);
      // The compiler-generated destructor is fine for non-base
      // classes without bare pointers or other resource use.
    
      // Plugins should not be copied or assigned.
      CRTRawDecoder(CRTRawDecoder const &) = delete;
      CRTRawDecoder(CRTRawDecoder &&) = delete;
      CRTRawDecoder & operator = (CRTRawDecoder const &) = delete;
      CRTRawDecoder & operator = (CRTRawDecoder &&) = delete;
    
      // Required functions.
      void produce(art::Event & e) override;
    
      // Selected optional functions.  I might get services that I expect to change here.  So far, seems like only service I might need is timing.
      void beginJob() override;
      //void beginRun(art::Run & r) override;
      //void beginSubRun(art::SubRun & sr) override;
    
    private:
    
      // Configuration data
      const art::InputTag fFragTag; //Label and product instance name (try eventdump.fcl on your input file) of the module that produced 
                                      //artdaq::Fragments from CRT data that I will turn into CRT::Triggers. Usually seems to be "daq" from artdaq.  

      const bool fLookForContainer; //Should this module look for "container" artdaq::Fragments instead of "single" artdaq::Fragments?  
                                    //If you ran the CRT board reader with the "request_mode" FHICL parameter set to "window", you need 
                                    //to look for "container" Fragments and probably won't get any useful Fragments otherwise.  If you 
                                    //ran the CRT board reader with "request_mode" set to "ignore", then set fLookForContainer to "false" 
                                    //which is the default.  
                                    //You probably want this set to "true" because the CRT should be run in "window" request_mode for 
                                    //production.  You might set it to "false" if you took debugging data in which you wanted to ignore 
                                    //timestamp matching with other detectors.  
      const bool fMatchOfflineMapping; //Should the hardware channel mapping match the order in which AuxDetGeos are sorted in the offline 
                                       //framework?  Default is true.  Please set to false for online monitoring so that CRT experts can 
                                       //understand problems more quickly.  
      std::vector<size_t> fChannelMap; //Simple map from raw data module number to offline module number.  Initialization depends on 
                                       //fMatchOfflineMapping above.

      // Compartmentalize internal functionality so that I can reuse it with both regular Fragments and "container" Fragments
      void FragmentToTriggers(const artdaq::Fragment& artFrag, std::unique_ptr<std::vector<CRT::Trigger>>& triggers);

      // For the first Event of every job, I want to set fEarliestTime to the earliest time in that Event
      void SetEarliestTime(const artdaq::Fragment& frag);

      //Sync diagnostic plots
      
      
    /* struct PerModule
      {
        PerModule(art::TFileDirectory& parent, const size_t module): fModuleDir(parent.mkdir("Module"+std::to_string(module)))
        {
          fLowerTimeVersusTime = fModuleDir.makeAndRegister<TGraph>("LowerTimeVersusTime", "Raw 32 Bit Timestamp versus Elapsed Time in Seconds;"
                                                                                           "Time [s];Raw Timestamp [ticks]");
        }

        art::TFileDirectory fModuleDir; //Directory for plots from this module
        TGraph* fLowerTimeVersusTime; //Graph of lower 32 bits of raw timestamp versus processed timestamp in seconds.  

      };
      void createSyncPlots(); 
      std::vector<PerModule> fSyncPlots; //Mapping from module number to sync diagnostic plots in a directory 
    */
      uint64_t fEarliestTime; //Earliest time in clock ticks
      
      

      


  };
  
  
  CRTRawDecoder::CRTRawDecoder(fhicl::ParameterSet const & p): EDProducer{p}, fFragTag(p.get<std::string>("RawDataTag")), 
                                                               fLookForContainer(p.get<bool>("LookForContainer", false)),
                                                               fMatchOfflineMapping(p.get<bool>("MatchOfflineMapping", true)),
                                                               fEarliestTime(std::numeric_limits<decltype(fEarliestTime)>::max())
  {
    // Call appropriate produces<>() functions here.
    produces<std::vector<CRT::Trigger>>();
    consumes<std::vector<artdaq::Fragment>>(fFragTag);
 
    if (p.has_key("metrics")) {
    sbndaq::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
    }
  //if (p.has_key("metric_config")) {
  //  sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_config"));
  //}
  //sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics")); //This causes the error for no "metrics" at the beginning or the end
    sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_channel_config"));
    //sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_board_config"));  //This line cauess the code to not be able to compile
    //art::ServiceHandle<art::TFileService> tfs;
    //tfs->registerFileSwitchCallback(this, &CRTRawDecoder::createSyncPlots);
  }

  void CRTRawDecoder::FragmentToTriggers(const artdaq::Fragment& artFrag, std::unique_ptr<std::vector<CRT::Trigger>>& triggers)
  {
    CRT::Fragment frag(artFrag);                                                                                                                                                   
    TLOG(TLVL_DEBUG,"CRT") << "Is this Fragment good?  " << ((frag.good_event())?"true":"false") << "\n";
    /*frag.print_header();
    frag.print_hits();*/
                                                                                                                                                   
    std::vector<CRT::Hit> hits;
    //Make Channel Mapping array
    size_t map_array[64] = {};
    int count = 1;
    //for (int i = 64; i>= 1; i--){ //Fills map_array with the correct channel mapping for the ICARUS bottom CRT
//	map_array[i] = count;
//	count++; 
 //   }
     //Alternative mapping

    int even = 64;
    int odd = 63;
    for (int i = 1; i<= 64; i++){

     if (count <= 4) {
     map_array [i] = even;
     even-=2;
     count++;
     }
     else if (count <= 8){
      map_array[i] = odd;
      odd-=2;
      if (count != 8){
      count++;
      }
      else{
      count = 1;
      }
     }
   }
  
    //Make a CRT::Hit from each non-zero ADC value in this Fragment
    for(size_t hitNum = 0; hitNum < frag.num_hits(); ++hitNum)
    {
      const auto hit = *(frag.hit(hitNum));
      //Metrics per channel  
      //
      //	
      //std::cout<<"Metrics Sent: " << "Channel: "<<std::to_string(hit.channel)<<"ADC: "<< std::to_string(hit.adc) <<'\n'; 
      //sbndaq::sendMetric("CRT_channel_bottom", std::to_string(hit.channel), "ADC", std::to_string(hit.adc), 0, artdaq::MetricMode::Average);
      //MF_LOG_DEBUG("CRTRaw") << "Channel: " << (int)(hit.channel) << "\n"
      //                    << "ADC: " << hit.adc << "\n";
      //Determine the offline channel number for each strip
      size_t offline_channel = hit.channel;
      /*if (frag.module_num() == 14 ||
          frag.module_num() == 15 ||
          frag.module_num() == 8 ||
          frag.module_num() == 9 ||
          frag.module_num() == 10 ||
          frag.module_num() == 11 ||
          //frag.module_num() == 4 ||
          //frag.module_num() == 5 ||
          frag.module_num() == 30 ||
          frag.module_num() == 31 ||
          frag.module_num() == 24 ||
          frag.module_num() == 25 ||
          frag.module_num() == 26 ||
          frag.module_num() == 27 ||
          frag.module_num() == 20 ||
          frag.module_num() == 21){ //Strips need to be flipped (TY)
        if (hit.channel<32){
          offline_channel = (31-hit.channel)*2;
        }
        else{
          offline_channel = (63-hit.channel)*2+1;
        }
      }
      */      
      //else{//Strips do not need to be flipped TODO: Fix channels
        offline_channel = map_array[hit.channel]; 
	//if (hit.channel<32){
        //  offline_channel = hit.channel*2;
        //}
        //else{
        //  offline_channel = (hit.channel-32)*2+1;
        //}
      //}
      //Flip the two layers
      //if (offline_channel%2==0) ++offline_channel;
      //else --offline_channel;
      hits.emplace_back(offline_channel, hit.adc);
      //MF_LOG_DEBUG("CRT Hits") CRT::operator << hits.back() << "\n"; //TODO: Some function template from the message service interferes with my  
                                                                    //      function template from namespace CRT.  using namespace CRT seems like 
                                                                    //      it should solve this, but it doesn't seem to.
    }
                                                                                                                                                   
    
    //TLOG(TLVL_WARNING,"CRT")  << "Module: " << frag.module_num() << "\n"
    //                          << "Number of hits: " << frag.num_hits() << "\n"
    //                          << "Fifty MHz time: " << frag.fifty_mhz_time() << "\n"
    //                          << "Fifty MHz time times tick: " << (uint64_t)(frag.fifty_mhz_time()*16) << "\n";
   
   
    try
    {  
      triggers->emplace_back(fChannelMap.at(frag.module_num()), frag.fifty_mhz_time(), std::move(hits)); 
    }
    catch(const std::out_of_range& e)
    {
       TLOG(TLVL_WARNING,"CRT")<< "Got CRT channel number " << frag.module_num() << " that is greater than the number of boards"
                                        << " in the channel map: " << fChannelMap.size() << ".  Throwing out this Trigger.\n";
    }
    
    //Make diagnostic plots for sync pulses
    //AC:
    /*

    const auto& plots = fSyncPlots[frag.module_num()];
    const double deltaT = (frag.fifty_mhz_time() - fEarliestTime)*1.6e-8; //TODO: Get size of clock ticks from a service
    if(deltaT > 0 && deltaT < 1e6) //Ignore time differences less than 1s and greater than 1 day
                                   //TODO: Understand why these cases come up
    {
      plots.fLowerTimeVersusTime->SetPoint(plots.fLowerTimeVersusTime->GetN(), deltaT, frag.raw_backend_time());
    }
    else
    {
      mf::LogWarning("SyncPlots") << "Got time difference " << deltaT << " that was not included in sync plots.\n"
                                  << "lhs is " << frag.fifty_mhz_time() << ", and rhs is " << fEarliestTime << ".\n"
                                  << "Hardware raw time is " << frag.raw_backend_time() << ".\n";
    }
    
    */

  }

  void CRTRawDecoder::SetEarliestTime(const artdaq::Fragment& frag)
  {
    CRT::Fragment crt(frag);
    if(crt.fifty_mhz_time() < fEarliestTime) fEarliestTime = crt.fifty_mhz_time();
  }
  
  //Read artdaq::Fragments produced by fFragTag, and use CRT::Fragment to convert them to CRT::Triggers.  
  void CRTRawDecoder::produce(art::Event & e)
  {
    //Create an empty container of CRT::Triggers.  Any Triggers in this container will be put into the 
    //event at the end of produce.  I will try to fill this container, but just not produce any CRT::Triggers 
    //if there are no input artdaq::Fragments.  
    auto triggers = std::make_unique<std::vector<CRT::Trigger>>();

    try
    {
      //Try to get artdaq::Fragments produced from CRT data.  The following line is the reason for 
      //this try-catch block.  I don't expect anything else to throw a cet::Exception.
      const auto& fragHandle = e.getValidHandle<std::vector<artdaq::Fragment>>(fFragTag);

      if(fLookForContainer)
      {
        //If this is the first event, set fEarliestTime
        if(fEarliestTime == std::numeric_limits<decltype(fEarliestTime)>::max())
        {
          for(const auto& frag: *fragHandle) 
          {
            artdaq::ContainerFragment container(frag);
            for(size_t pos = 0; pos < container.block_count(); ++pos) SetEarliestTime(*container[pos]);
          }
        }

        for(const auto& artFrag: *fragHandle)
        {
          artdaq::ContainerFragment container(artFrag);
          for(size_t pos = 0; pos < container.block_count(); ++pos) FragmentToTriggers(*container[pos], triggers); 
        }
      }
      else
      {
        //If this is the first event, set fEarliestTime
        if(fEarliestTime == std::numeric_limits<decltype(fEarliestTime)>::max())
        {
          for(const auto& frag: *fragHandle) SetEarliestTime(frag);
        }

        //Convert each fragment into a CRT::Trigger.
        for(const auto& artFrag: *fragHandle) FragmentToTriggers(artFrag, triggers);
      }
    }
    catch(const cet::exception& exc) //If there are no artdaq::Fragments in this Event, just add an empty container of CRT::Triggers.
    {
       TLOG(TLVL_WARNING,"CRT") << "No artdaq::Fragments produced by " << fFragTag << " in this event, so "
                                    << "not doing anything.\n";
    }

    //Put a vector of CRT::Triggers into this Event for other modules to read.
    e.put(std::move(triggers));
  }
  
  void CRT::CRTRawDecoder::beginJob()
  {
    // createSyncPlots();
  
    //Set up channel mapping base on user configuration
    //AC: art::ServiceHandle<geo::Geometry> geom;
    //AC: const auto nModules = geom->NAuxDets();
    const size_t nModules = 56; //14; 
    if(fMatchOfflineMapping) 
    {
//      std::vector<const geo::AuxDetGeo*> auxDets; //The function to retrieve this vector seems to have been made protected
//      for(size_t auxDet = 0; auxDet < nModules; ++auxDet) auxDets.push_back(&geom->AuxDet(auxDet));
//
//      fChannelMap = CRTSorter::Mapping(auxDets); //Match whatever the current AuxDet sorter does.  
      //Based on v6 geometry (TY)
      fChannelMap.push_back(24);
      fChannelMap.push_back(25);
      fChannelMap.push_back(30);
      fChannelMap.push_back(18);
      fChannelMap.push_back(15);
      fChannelMap.push_back(7);
      fChannelMap.push_back(13);
      fChannelMap.push_back(12);
      fChannelMap.push_back(11);
      fChannelMap.push_back(10);
      fChannelMap.push_back(6);
      fChannelMap.push_back(14);
      fChannelMap.push_back(19);
      fChannelMap.push_back(31);
      fChannelMap.push_back(26);
      fChannelMap.push_back(27);
      fChannelMap.push_back(22);
      fChannelMap.push_back(23);
      fChannelMap.push_back(29);
      fChannelMap.push_back(17);
      fChannelMap.push_back(8);
      fChannelMap.push_back(0);
      fChannelMap.push_back(3);
      fChannelMap.push_back(2);
      fChannelMap.push_back(5);
      fChannelMap.push_back(4);
      fChannelMap.push_back(1);
      fChannelMap.push_back(9);
      fChannelMap.push_back(16);
      fChannelMap.push_back(28);
      fChannelMap.push_back(20);
      fChannelMap.push_back(21);
    }
    else 
    {
      for(size_t module = 0; module < nModules; ++module) fChannelMap.push_back(module); //Map each index to itself
    }
  }

  // void CRT::CRTRawDecoder::createSyncPlots()
  //{
    //const auto nModules = art::ServiceHandle<geo::Geometry>()->NAuxDets();
    //art::ServiceHandle<art::TFileService> tfs;
    //fSyncPlots.clear(); //Clear any previous pointers to histograms.  Some TFile owned them.  
    //for(size_t module = 0; module < nModules; ++module) fSyncPlots.emplace_back(*tfs, module); //32 modules in the ProtoDUNE-SP CRT
  //}
  
  /*void CRT::CRTRawDecoder::beginRun(art::Run & r)
  {
    // Implementation of optional member function here.
  }
  
  void CRT::CRTRawDecoder::beginSubRun(art::SubRun & sr)
  {
    // Implementation of optional member function here.
  }*/
}

DEFINE_ART_MODULE(CRT::CRTRawDecoder)
