////////////////////////////////////////////////////////////////////////
// Class:       Hitdumper
// Module Type: analyzer
// File:        Hitdumper_module.cc
//
// Generated at Sun Mar 24 09:05:02 2013 by Tingjun Yang using artmod
// from art v1_02_06.
////////////////////////////////////////////////////////////////////////
#ifndef Hitdumper_Module
#define Hitdumper_Module

// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Principal/Event.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Handle.h" 
#include "canvas/Persistency/Common/Ptr.h" 
#include "canvas/Persistency/Common/PtrVector.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h" 
#include "art_root_io/TFileService.h" 
#include "art_root_io/TFileDirectory.h" 
#include "messagefacility/MessageLogger/MessageLogger.h" 

// LArSoft includes
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "lardataobj/RecoBase/Hit.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcore/Geometry/Geometry.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorClocksService.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcore/Geometry/AuxDetGeometry.h"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTData.hh"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTHit.hh"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTTrack.hh"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTHitRecoAlg.h"

// ROOT includes
#include "TTree.h"
#include "TTimeStamp.h"
#include "TH1.h"
#include "TH2.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include "TGeoManager.h"


// C++ Includes
#include <map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <cmath>

const int kMaxHits       = 30000; //maximum number of hits
//const int kMaxAuxDets = 100;
//const unsigned short kMaxTkIDs = 100;
const int kMaxCHits       = 1000; //maximum number of hits
const int kMaxMCpart       = 10; //maximum number of hits
const int kMaxNCtrks     = 10;

namespace MyHitdumper {

  class Hitdumper : public art::EDAnalyzer {
  public:
    explicit Hitdumper(fhicl::ParameterSet const & p);
    virtual ~Hitdumper();

    // This method is called once, at the start of the job. In this
    // example, it will define the histograms and n-tuples we'll write.
    void beginJob();

    // This method is called once, at the start of each run. It's a
    // good place to read databases or files that may have
    // run-dependent information.
    //    void beginRun(const art::Run& run);

    // This method reads in any parameters from the .fcl files. This
    // method is called 'reconfigure' because it might be called in the
    // middle of a job; e.g., if the user changes parameter values in an
    // interactive event display.
    void reconfigure(fhicl::ParameterSet const& pset);

    // The analysis routine, called once per event. 
    void analyze (const art::Event& evt); 

  private:

    void ResetVars();
  
    TTree* fTree;
    //run information
    int run;
    int subrun;
    int event;
    double evttime;
    int t0;

    int    nhits;
    int    hit_cryostat[kMaxHits];
    int    hit_tpc[kMaxHits];
    int    hit_plane[kMaxHits];
    int    hit_wire[kMaxHits];
    int    hit_channel[kMaxHits];
    double hit_peakT[kMaxHits];
    double hit_charge[kMaxHits];
    double hit_ph[kMaxHits];
    double hit_width[kMaxHits];

    int nstrips;
    int crt_plane[kMaxCHits];
    int crt_module[kMaxCHits];
    int crt_strip[kMaxCHits];
    int crt_orient[kMaxCHits];
    double crt_time[kMaxCHits];
    double crt_adc[kMaxCHits];
    double crt_pos[kMaxCHits];

    int nctrks;
    double ctrk_x1[kMaxNCtrks];
    double ctrk_y1[kMaxNCtrks];
    double ctrk_z1[kMaxNCtrks];
    double ctrk_t1[kMaxNCtrks];
    double ctrk_adc1[kMaxNCtrks];
    int ctrk_mod1x[kMaxNCtrks];
    double ctrk_x2[kMaxNCtrks];
    double ctrk_y2[kMaxNCtrks];
    double ctrk_z2[kMaxNCtrks];
    double ctrk_t2[kMaxNCtrks];
    double ctrk_adc2[kMaxNCtrks];
    int ctrk_mod2x[kMaxNCtrks];
  
    int nchits;
    double chit_x[kMaxCHits];
    double chit_y[kMaxCHits];
    double chit_z[kMaxCHits];
    double chit_time[kMaxCHits];
    double chit_adc[kMaxCHits];
    int chit_plane[kMaxCHits];

    int ncts;
    double ct_time[kMaxNCtrks];
    double ct_pes[kMaxNCtrks];
    double ct_x1[kMaxNCtrks];
    double ct_y1[kMaxNCtrks];
    double ct_z1[kMaxNCtrks];
    double ct_x2[kMaxNCtrks];
    double ct_y2[kMaxNCtrks];
    double ct_z2[kMaxNCtrks];

    // int    nmcpart;
    // double mc_time[kMaxMCpart];

    std::string fHitsModuleLabel;
    std::string fLArG4ModuleLabel;
    std::string fCRTStripModuleLabel;
    std::string fCRTHitModuleLabel;
    std::string fCRTTrackModuleLabel;
    double fSelectedPDG;

    bool fkeepCRThits;
    bool fkeepCRTstrips;
    bool fmakeCRTtracks;
    bool freadCRTtracks;


    sbndqm::CRTHitRecoAlg hitAlg;

    geo::GeometryCore const* fGeometryService;
    // detinfo::DetectorClocks const* fDetectorClocks;
    // detinfo::DetectorProperties const* fDetectorProperties;
    // detinfo::ElecClock fTrigClock;
    art::ServiceHandle<geo::AuxDetGeometry> fAuxDetGeoService;
    const geo::AuxDetGeometry* fAuxDetGeo;
    const geo::AuxDetGeometryCore* fAuxDetGeoCore;

  };


  Hitdumper::Hitdumper(fhicl::ParameterSet const& pset)
    : EDAnalyzer(pset)
  {



    fGeometryService = lar::providerFrom<geo::Geometry>();
    // fDetectorClocks = lar::providerFrom<detinfo::DetectorClocksService>();
    // fDetectorProperties = lar::providerFrom<detinfo::DetectorPropertiesService>();
    // fTrigClock = fDetectorClocks->TriggerClock();
    fAuxDetGeo = &(*fAuxDetGeoService);
    fAuxDetGeoCore = fAuxDetGeo->GetProviderPtr();

    // Read in the parameters from the .fcl file.
    this->reconfigure(pset);
  }
  
  void Hitdumper::reconfigure(fhicl::ParameterSet const& p)
  {   // Read parameters from the .fcl file. The names in the arguments
    // to p.get<TYPE> must match names in the .fcl file.


    //  hitAlg(p.get<fhicl::ParameterSet>("HitAlg"));


    fHitsModuleLabel = p.get< std::string >("HitsModuleLabel");
    fLArG4ModuleLabel = "largeant";
    fCRTStripModuleLabel = p.get< std::string >("CRTStripModuleLabel","crt");
    fCRTHitModuleLabel = p.get< std::string >("CRTHitModuleLabel","crthit");
    fCRTTrackModuleLabel = p.get< std::string >("CRTTrackModuleLabel","crttrack");
    //    fLArG4ModuleLabel(p.get< std::string >("LArGeantModuleLabel", "largeant"));
    // hardcode the desired PDG value for now
    //  fSelectedPDG             = p.get< int         >("PDGcode");
    fSelectedPDG             = 13;
    fkeepCRThits = p.get< bool >("keepCRThits",true);
    fkeepCRTstrips = p.get<bool>("keepCRTstrips",false);
    fmakeCRTtracks= p.get< bool >("makeCRTtracks",true);
    freadCRTtracks= p.get< bool >("readCRTtracks",true);

    return;
  }
  



  Hitdumper::~Hitdumper()
  {
    // Clean up dynamic memory and other resources here.
  }

  void Hitdumper::analyze(const art::Event& evt)
  {
    // Implementation of required member function here.
    ResetVars();




    run = evt.run();
    subrun = evt.subRun();
    event = evt.id().event();


    /*
    // MC truth
    art::Handle< std::vector<simb::MCParticle> > particleHandle;
    //  event.getByLabel(fSimulationProducerLabel, particleHandle);  
    evt.getByLabel( fLArG4ModuleLabel, particleHandle);  


    nmcpart=0;
    // Loop over MC particles
    for ( auto const& particle : (*particleHandle) ) {
    
    // Fill the tree
    // only with information from the primary particles in the
    // event, whose PDG codes match a value supplied in the .fcl file.
    //
    // THIS WON'T WORK FOR 10-drift-window CRY samples !!!
    //
    float fPDG = particle.PdgCode();
    if ( particle.Process() == "primary"  &&  fPDG == fSelectedPDG ) {
    mc_time[nmcpart] = particle.T(0);
    nmcpart++;
    // std::cout << " Event " << event << " MCpart " << nmcpart << 
    // 	" Px= " << particle.Px() <<
    // 	" Py= " << particle.Py() <<
    // 	" Pz= " << particle.Pz() << std::endl;
    }
    }
    //  std::cout << "no mc part " << nmcpart << std::endl;
    */


    t0=0.;
    //  t0 = detprop->TriggerOffset();  // units of TPC ticks

    //*  Hits
    art::Handle< std::vector<recob::Hit> > hitListHandle; // Define a handle for recob::Hit for storing the hitList data.
    std::vector<art::Ptr<recob::Hit> > hitlist;            //get the recob::Hit using art::Ptr<T>
    if (evt.getByLabel(fHitsModuleLabel,hitListHandle)) {
      art::fill_ptr_vector(hitlist, hitListHandle);
      nhits = hitlist.size(); //Number of hits
    }
    else nhits=0;
    //  std::cout << " number TPC hits " << nhits << std::endl;
    //TPC data
    if (nhits>kMaxHits) nhits=kMaxHits;  //if nhits is more than 30k, the use the new no
    for (int i = 0; i<nhits; ++i){  //loop over all hits
      geo::WireID wireid = hitlist[i]->WireID(); //Initial tdc tick for hit.
      hit_cryostat[i]   = wireid.Cryostat; 
      hit_tpc[i]   = wireid.TPC; //this is the same as hit_tpc[i] = hitlist[i]->WireID().TPC, WireID is of type geo::WireID
      hit_plane[i]   = wireid.Plane; //this is the same as hit_tpc[i] = hitlist[i]->WireID().Plane (use to identify a plane)
      hit_wire[i]    = wireid.Wire;  //this is the same as hit_tpc[i] = hitlist[i]->WireID().Wire
      hit_channel[i] = hitlist[i]->Channel();
      // peak time needs plane dependent offset correction applied.
      hit_peakT[i]   = hitlist[i]->PeakTime(); //in tick units
      hit_charge[i]  = hitlist[i]->Integral(); //in tick x ADC units
      hit_ph[i]      = hitlist[i]->PeakAmplitude(); //in ADC units.
      hit_width[i]      = hitlist[i]->RMS();
      std::cout<<"hitlist[i]: "<<hitlist[i]<<" ,hitListHandle: "<<hitListHandle <<std::endl;
      std::cout<<"WireID(): "<< hitlist[i]->WireID()<<" ,wireid.Cryostat: "<<wireid.Cryostat<<" ,wireid.TPC: "<<wireid.TPC<<" ,wireid.Plane: "<<wireid.Plane<<" ,wireid.Wire: "<<wireid.Wire<<std::endl;
      std::cout<<"PeakTime(): " <<hitlist[i]->PeakTime()<<" ,Integral(): "<<hitlist[i]->Integral()<<" ,PeakAmplitude(): "<<hitlist[i]->PeakAmplitude()<<" ,RMS(): "<<hitlist[i]->RMS()<<std::endl;
    }
  
    // CRT strips
    int nstr=0;
    art::Handle<std::vector<sbndqm::crt::CRTData> > crtStripListHandle; //Handle for filling the sbndqm CRTData in the art tree 
    std::vector<art::Ptr<sbndqm::crt::CRTData> > striplist;             //Vector that has all the sbndqm CRTData
    // art::Handle< std::vector<crt::CRTData> > crtStripListHandle;
    // std::vector< art::Ptr<crt::CRTData> > striplist;
    if (evt.getByLabel(fCRTStripModuleLabel, crtStripListHandle))  {
      art::fill_ptr_vector(striplist, crtStripListHandle); //
      nstr = striplist.size();  //Number of crtStripHits
    }
  
    int ns =0;
    if (nstr>kMaxCHits) nstr=kMaxCHits;
    for (int i = 0; i<nstr; i+=2){
      uint32_t chan = striplist[i]->Channel();

      //    std::pair<std::string,unsigned> tagger = CRTHitRecoAlg::ChannelToTagger(chan);
      std::pair<std::string,unsigned> tagger = hitAlg.ChannelToTagger(chan);
      int ip=-1;
      if  (tagger.first=="volTaggerFaceFront_0" ) ip=1; 
      else if (tagger.first=="volTaggerFaceBack_0") ip=2; 

      if (ip>=0) {

	uint32_t ttime = striplist[i]->T0();
	//  T0 in units of ticks, but clock frequency (16 ticks = 1 us) is wrong.
	// ints were stored as uints, need to patch this up
	float ctime = ttime/16.;
	if (ttime > 2147483648) {
	  ctime = ((ttime-4294967296)/16.);
	}
	if (ip>=0 && ctime < 1600. && ctime > -1400. ) {
	  uint32_t adc1 = striplist[i]->ADC();
	  uint32_t adc2 = striplist[i+1]->ADC();
	  if (adc1>4095) adc1=4095;
	  if (adc2>4095) adc2=4095;
	  //    std::cout << tagger.first << " " << tagger.second << std::endl;
	  //    int sipm = chan & 1;  // 0 or 1
	  int strip = (chan >> 1) & 15;
	  int module = (chan>> 5);
	  //
	  std::string name = fGeometryService->AuxDet(module).TotalVolume()->GetName();
	  TVector3 center = fAuxDetGeoCore->AuxDetChannelToPosition(2*strip, name);
	  crt_plane[ns]=ip;
	  crt_module[ns]=module;
	  crt_strip[ns]=strip;
	  crt_orient[ns]=tagger.second;  // =0 for y (horizontal)  and =1 for x (vertical)
	  crt_time[ns]=ctime;
	  crt_adc[ns]=adc1+adc2-127.2; // -127.2/131.9 correct for gain and 2*ped to get pe
	  crt_pos[ns]=center.Y();
	  if (tagger.second==1) 	crt_pos[ns]=center.X();
	  ns++;

	  //      std::cout << ip << " " << tagger.second << " " << chan << " " << module << " " << strip << " " << center.X() << " " 
	  //	<< center.Y() << " " << center.Z() << " " << adc1+adc2 << " " << 	ctime << std::endl; 
	}
      }
    }
    nstrips=ns;

    nctrks=0;
    if (fmakeCRTtracks) {
      int ntr = 0;
      int iflag[1000] = {0};
      for (int i=0;i<(ns-1);++i) {
	if (iflag[i]==0) {
	  iflag[i]=1;
	  float plane1x =0; float plane2x = 0;
	  float plane1y =0; float plane2y = 0;
	  float plane1tx =0; float plane2tx = 0;
	  float plane1ty =0; float plane2ty = 0;
	  float plane1xm =-1; float plane2xm = -1;
	  float plane1ym =-1; float plane2ym = -1;
	  int  nh1x=0; int nh2x = 0;
	  int  nh1y=0; int nh2y = 0;
	  float adc1x=0; float adc2x=0;
	  float adc1y=0; float adc2y=0;
	  if (crt_plane[i]==1) {
	    if (crt_orient[i]==1 && crt_adc[i]>500) {
	      if (nh1x==0 ||  (crt_module[i]==plane1xm)) {
		nh1x++;	  
		if (crt_adc[i]>adc1x) {
		  plane1tx=crt_time[i];
		  adc1x+=crt_adc[i];
		  plane1x=crt_pos[i];
		  plane1xm=crt_module[i];
		}
	      }
	    }
	    else if (crt_orient[i]==0 && crt_adc[i]>500) {
	      if (nh1y==0 ||  (crt_module[i]==plane1ym)) {
		nh1y++; 
		if (crt_adc[i]>adc1y) {
		  plane1ty=crt_time[i];
		  adc1y+=crt_adc[i];
		  plane1y=crt_pos[i];
		  plane1ym=crt_module[i];
		}
	      }
	    }
	  }
	  else {
	    if (crt_orient[i]==1 && crt_adc[i]>500) {
	      if (nh2x==0 ||  (crt_module[i]==plane2xm)) {
		nh2x++;	  
		if (crt_adc[i]>adc2x) {
		  plane2tx=crt_time[i];
		  adc2x+=crt_adc[i];
		  plane2x=crt_pos[i];
		  plane2xm=crt_module[i];
		}
	      }
	    }
	    else if (crt_orient[i]==0 && crt_adc[i]>500) {
	      if (nh2y==0 ||  (crt_module[i]==plane2ym)) {
		nh2y++; 
		if (crt_adc[i]>adc2y) {
		  plane2ty=crt_time[i];
		  adc2y+=crt_adc[i];
		  plane2y=crt_pos[i];
		  plane2ym=crt_module[i];
		}
	      }
	    }
	  }
	  for (int j=i+1;j<ns;++j) {
	    float tdiff = fabs(crt_time[i]-crt_time[j]);
	    if (tdiff<0.1) {
	      iflag[j]=1;
	      if (crt_plane[j]==1) {
		if (crt_orient[j]==1 && crt_adc[j]>1000) {
		  if (nh1x==0 ||  (crt_module[j]==plane1xm)) {
		    nh1x++;	  
		    if (crt_adc[j]>adc1x) {
		      plane1tx=crt_time[j];
		      adc1x+=crt_adc[j];
		      plane1x=crt_pos[j];
		      plane1xm=crt_module[j];
		    }
		  }
		}
		else if (crt_orient[j]==0 && crt_adc[j]>1000) {
		  if (nh1y==0 ||  (crt_module[j]==plane1ym)) {
		    nh1y++; 
		    if (crt_adc[j]>adc1y) {
		      plane1ty=crt_time[j];
		      adc1y+=crt_adc[j];
		      plane1y=crt_pos[j];
		      plane1ym=crt_module[j];
		    }
		  }
		}
	      }
	      else {
		if (crt_orient[j]==1 && crt_adc[j]>1000) {
		  if (nh2x==0 ||  (crt_module[j]==plane2xm)) {
		    nh2x++;	  
		    if (crt_adc[j]>adc2x) {
		      plane2tx=crt_time[j];
		      adc2x+=crt_adc[j];
		      plane2x=crt_pos[j];
		      plane2xm=crt_module[j];
		    }
		  }
		}
		else if (crt_orient[j]==0 && crt_adc[j]>1000) {
		  if (nh2y==0 ||  (crt_module[j]==plane2ym)) {
		    nh2y++; 
		    if (crt_adc[j]>adc2y) {
		      plane2ty=crt_time[j];
		      adc2y+=crt_adc[j];
		      plane2y=crt_pos[j];
		      plane2ym=crt_module[j];
		    }
		  }
		}
	      }
	    }
	  } // look for hits at the same time as hit i
	  if (nh1x>0 && nh1y>0 && nh2x>0 && nh2y>0 && adc1x<9000 && adc1y<9000 && adc2x<9000 && adc2y<9000) { 
	    // make a track!
	    ctrk_x1[ntr]=plane1x;
	    ctrk_y1[ntr]=plane1y;
	    ctrk_z1[ntr]=-239.95;
	    ctrk_t1[ntr]=0.5*(plane1tx+plane1ty);
	    ctrk_adc1[ntr]=adc1x+adc1y;
	    ctrk_mod1x[ntr]=plane1xm;
	    ctrk_x2[ntr]=plane2x;
	    ctrk_y2[ntr]=plane2y;
	    ctrk_z2[ntr]=656.25;
	    ctrk_t2[ntr]=0.5*(plane2tx+plane2ty);
	    ctrk_adc2[ntr]=adc2x+adc2y;
	    ctrk_mod2x[ntr]=plane2xm;
	    ntr++;
	    // std::cout << "track " << ntr << std::endl;
	    // std::cout <<  "x y t adc: plane 1 " << plane1x << " " << plane1y << " " << 
	    //   0.5*(plane1tx+plane1ty) << " " << adc1x << " " << adc1y << std::endl;
	    // std::cout <<  "         : plane 2 " << plane2x << " " << plane2y << " " << 
	    //   0.5*(plane2tx+plane2ty) << " " << adc2x << " " << adc2y << std::endl;
	  }
	} // i is the first hit with this time
      } // loop over hits      
      nctrks=ntr;
    }  // end if make tracks

    // CRT hits
    if (fkeepCRThits) {
      art::Handle<std::vector<sbndqm::crt::CRTHit> > crtHitListHandle;
      std::vector<art::Ptr<sbndqm::crt::CRTHit> > chitlist;
      // art::Handle< std::vector<crt::CRTData> > crtStripListHandle;
      // std::vector< art::Ptr<crt::CRTData> > striplist;
      if (evt.getByLabel(fCRTHitModuleLabel, crtHitListHandle))  {
	art::fill_ptr_vector(chitlist, crtHitListHandle);
	nchits = chitlist.size();//}
	///else nchits=0;
    
	if (nchits>kMaxCHits) nchits=kMaxCHits;
	//  std::cout << " number CRT hits " << nchits << std::endl;
	for (int i = 0; i<nchits; ++i){
	  int ip = -1;
	  if (chitlist[i]->tagger=="volTaggerBot_0") ip=0; 
	  else if (chitlist[i]->tagger=="volTaggerFaceFront_0") ip=1; 
	  else if (chitlist[i]->tagger=="volTaggerFaceBack_0") ip=2; 
	  else if (chitlist[i]->tagger=="volTaggerSideLeft_0") ip=3; 
	  else if (chitlist[i]->tagger=="volTaggerSideRight_0") ip=4; 
	  else if (chitlist[i]->tagger=="volTaggerTopLow_0") ip=5; 
	  else if (chitlist[i]->tagger=="volTaggerTopHigh_0") ip=6; 
	  chit_time[i]=chitlist[i]->ts1_ns*0.001;
	  chit_x[i]=chitlist[i]->x_pos;
	  chit_y[i]=chitlist[i]->y_pos;
	  chit_z[i]=chitlist[i]->z_pos;
	  chit_plane[i]=ip;
	}

      }

      else nchits=0;
    }

    // CRT tracks
    ncts=0;
    if (freadCRTtracks) {
      art::Handle<std::vector<sbndqm::crt::CRTTrack> > crtTrackListHandle;
      std::vector<art::Ptr<sbndqm::crt::CRTTrack> > ctrklist;
      if (evt.getByLabel(fCRTTrackModuleLabel, crtTrackListHandle))  {
	art::fill_ptr_vector(ctrklist, crtTrackListHandle);
	ncts =ctrklist.size();
	if (ncts>kMaxNCtrks) ncts=kMaxNCtrks;
	for (int i = 0; i<ncts; ++i){
	  ct_pes[i]=ctrklist[i]->peshit;
	  ct_time[i]=ctrklist[i]->ts1_ns*0.001;
	  ct_x1[i]=ctrklist[i]->x1_pos;
	  ct_y1[i]=ctrklist[i]->y1_pos;
	  ct_z1[i]=ctrklist[i]->z1_pos;
	  ct_x2[i]=ctrklist[i]->x2_pos;
	  ct_y2[i]=ctrklist[i]->y2_pos;
	  ct_z2[i]=ctrklist[i]->z2_pos;
	}
      }
    }

    fTree->Fill();
  
  }

  void Hitdumper::beginJob()
  {
    // Implementation of optional member function here.
    art::ServiceHandle<art::TFileService> tfs;
    fTree = tfs->make<TTree>("hitdumper","analysis tree");
    fTree->Branch("run",&run,"run/I");
    fTree->Branch("subrun",&subrun,"subrun/I");
    fTree->Branch("event",&event,"event/I");
    fTree->Branch("evttime",&evttime,"evttime/D");
    fTree->Branch("t0",&t0,"t0/I");
    fTree->Branch("nhits",&nhits,"nhits/I");
    fTree->Branch("hit_cryostat",hit_cryostat,"hit_cryostat[nhits]/I");
    fTree->Branch("hit_tpc",hit_tpc,"hit_tpc[nhits]/I");
    fTree->Branch("hit_plane",hit_plane,"hit_plane[nhits]/I");
    fTree->Branch("hit_wire",hit_wire,"hit_wire[nhits]/I");
    fTree->Branch("hit_channel",hit_channel,"hit_channel[nhits]/I");
    fTree->Branch("hit_peakT",hit_peakT,"hit_peakT[nhits]/D");
    fTree->Branch("hit_charge",hit_charge,"hit_charge[nhits]/D");
    fTree->Branch("hit_ph",hit_ph,"hit_ph[nhits]/D");
    fTree->Branch("hit_width",hit_width,"hit_width[nhits]/D");
    if (fkeepCRTstrips) {
      fTree->Branch("nstrips",&nstrips,"nstrips/I");
      fTree->Branch("crt_plane",crt_plane,"crt_plane[nstrips]/I");
      fTree->Branch("crt_module",crt_module,"crt_module[nstrips]/I");
      fTree->Branch("crt_strip",crt_strip,"crt_strip[nstrips]/I");
      fTree->Branch("crt_orient",crt_orient,"crt_orient[nstrips]/I");
      fTree->Branch("crt_time",crt_time,"crt_time[nstrips]/D");
      fTree->Branch("crt_adc",crt_adc,"crt_adc[nstrips]/D");
      fTree->Branch("crt_pos",crt_pos,"crt_pos[nstrips]/D");
    }
    if (fmakeCRTtracks) {
      fTree->Branch("nctrks",&nctrks,"nctrks/I");
      fTree->Branch("ctrk_x1",ctrk_x1,"ctrk_x1[nctrks]/D");
      fTree->Branch("ctrk_y1",ctrk_y1,"ctrk_y1[nctrks]/D");
      fTree->Branch("ctrk_z1",ctrk_z1,"ctrk_z1[nctrks]/D");
      fTree->Branch("ctrk_t1",ctrk_t1,"ctrk_t1[nctrks]/D");
      fTree->Branch("ctrk_adc1",ctrk_adc1,"ctrk_adc1[nctrks]/D");
      fTree->Branch("ctrk_mod1x",ctrk_mod1x,"ctrk_mod1x[nctrks]/I");
      fTree->Branch("ctrk_x2",ctrk_x2,"ctrk_x2[nctrks]/D");
      fTree->Branch("ctrk_y2",ctrk_y2,"ctrk_y2[nctrks]/D");
      fTree->Branch("ctrk_z2",ctrk_z2,"ctrk_z2[nctrks]/D");
      fTree->Branch("ctrk_t2",ctrk_t2,"ctrk_t2[nctrks]/D");
      fTree->Branch("ctrk_adc2",ctrk_adc2,"ctrk_adc2[nctrks]/D");
      fTree->Branch("ctrk_mod2x",ctrk_mod2x,"ctrk_mod2x[nctrks]/I");
    }
    if (fkeepCRThits) {
      fTree->Branch("nchits",&nchits,"nchits/I");
      fTree->Branch("chit_x",chit_x,"chit_x[nchits]/D");
      fTree->Branch("chit_y",chit_y,"chit_y[nchits]/D");
      fTree->Branch("chit_z",chit_z,"chit_z[nchits]/D");
      fTree->Branch("chit_time",chit_time,"chit_time[nchits]/D");
      fTree->Branch("chit_plane",chit_plane,"chit_plane[nchits]/I");    
    }
    if (freadCRTtracks) {
      fTree->Branch("ncts",&ncts,"ncts/I");
      fTree->Branch("ct_x1",ct_x1,"ct_x1[ncts]/D");
      fTree->Branch("ct_y1",ct_y1,"ct_y1[ncts]/D");
      fTree->Branch("ct_z1",ct_z1,"ct_z1[ncts]/D");
      fTree->Branch("ct_x2",ct_x2,"ct_x2[ncts]/D");
      fTree->Branch("ct_y2",ct_y2,"ct_y2[ncts]/D");
      fTree->Branch("ct_z2",ct_z2,"ct_z2[ncts]/D");
      fTree->Branch("ct_time",ct_time,"ct_time[ncts]/D");
      fTree->Branch("ct_pes",ct_pes,"ct_pes[ncts]/D");
    }
  
  }

  //void Hitdumper::reconfigure(fhicl::ParameterSet const & p)
  //{
  //  // Implementation of optional member function here.
  //}
  void Hitdumper::ResetVars(){

    run = -99999;
    subrun = -99999;
    event = -99999;
    evttime = -99999;
    t0 = -99999;
    nhits = 0;
    for (int i = 0; i<kMaxHits; ++i){
      hit_cryostat[i] = -99999;
      hit_tpc[i] = -99999;
      hit_plane[i] = -99999;
      hit_wire[i] = -99999;
      hit_channel[i] = -99999;
      hit_peakT[i] = -99999;
      hit_charge[i] = -99999;
      hit_ph[i] = -99999;
      hit_width[i] = -99999;
    }
    nstrips=0;
    for (int i=0;i<kMaxCHits;++i) {
      crt_plane[i]=-999;
      crt_module[i]=-999;
      crt_strip[i]=-999;
      crt_orient[i]=-999;
      crt_time[i]=-9999999.;
      crt_adc[i]=-999999.;
      crt_pos[i]=-999999.;
    }
  
    nctrks=0;
    for (int i=0;i<kMaxNCtrks;++i) {
      ctrk_x1[i]=-9999.;
      ctrk_y1[i]=-9999.;
      ctrk_z1[i]=-9999.;
      ctrk_t1[i]=-9999.;
      ctrk_adc1[i]=-9999.;
      ctrk_mod1x[i]=-99;
      ctrk_x2[i]=-9999.;
      ctrk_y2[i]=-9999.;
      ctrk_z2[i]=-9999.;
      ctrk_t2[i]=-9999.;
      ctrk_adc2[i]=-9999.;
      ctrk_mod2x[i]=-99;
    }

    ncts=0;
    for (int i=0;i<kMaxNCtrks;++i) {
      ct_x1[i]=-9999.;
      ct_y1[i]=-9999.;
      ct_z1[i]=-9999.;
      ct_time[i]=-9999.;
      ct_pes[i]=-9999.;
      ct_x2[i]=-9999.;
      ct_y2[i]=-9999.;
      ct_z2[i]=-9999.;
    }


    nchits=0;
    for (int i=0;i<kMaxCHits;++i) {
      chit_plane[i] = -999;
      chit_time[i] = -999.9;
      chit_x[i] = -999.9;
      chit_y[i] = -999.9;
      chit_z[i] = -999.9;
    }
    // nmcpart=0;
    // for (int i=0;i<kMaxMCpart;++i) {
    //   mc_time[i] = -999.9;
    // }
  
  }

  DEFINE_ART_MODULE(Hitdumper)

}  // namespace Hitdumper

#endif // Hitdumper_Module
