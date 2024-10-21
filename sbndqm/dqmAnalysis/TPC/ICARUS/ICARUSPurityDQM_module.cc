////////////////////////////////////////////////////////////////////////
//
// ICARUSPurityDQM class
//
// Christian Farnese
//
////////////////////////////////////////////////////////////////////////

//#ifndef ICARUSPURITYDQM_H
//#define ICARUSPURITYDQM_H

#include <iomanip>
#include <TH1F.h>
#include <TProfile.h>
#include <vector>
#include <string>
#include <array>
#include <fstream>

//Framework includes
#include "art/Framework/Core/ModuleMacros.h" 
#include "canvas/Persistency/Common/FindManyP.h"
#include "art/Framework/Principal/Event.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Handle.h" 
#include "canvas/Persistency/Common/Ptr.h" 
#include "canvas/Persistency/Common/PtrVector.h" 
//#include "art/Framework/Services/Registry/ServiceHandle.h" 
//#include "art/Framework/Services/Optional/TFileService.h" 
#include "art_root_io/TFileService.h"
//#include "art/Framework/Services/Optional/TFileDirectory.h" 
//#include "messagefacility/MessageLogger/MessageLogger.h" 

//LArSoft includes
#include "larcore/Geometry/Geometry.h"
#include "nusimdata/SimulationBase/MCTruth.h"
//#include "nutools/ParticleNavigation/ParticleList.h"
//#include "nutools/ParticleNavigation/EmEveIdCalculator.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Wire.h"
#include "lardataobj/RecoBase/Vertex.h"
// #include "RawData/RawDigit.h"
#include "larsim/MCCheater/BackTracker.h"
#include "lardata/Utilities/AssociationUtil.h"

#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/raw.h"



#include "art/Framework/Core/EDAnalyzer.h"
#include <TMath.h>
#include <TH1F.h>
#include "TH2D.h"
#include "TProfile2D.h"
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TGraphAsymmErrors.h>
#include "TF1.h"
#include "TCanvas.h"

class TH1F;
class TH2F;
///Cluster finding and building 
namespace cluster {

   
  class ICARUSPurityDQM : public art::EDAnalyzer {

  public:
          
    explicit ICARUSPurityDQM(fhicl::ParameterSet const& pset);
    virtual ~ICARUSPurityDQM();
 
    /// read access to event
    void analyze(const art::Event& evt);
    void beginJob();
    void endJob();
    int Nothere(std::vector<int>* a, int b);
    int Notheref(std::vector<float>* a, float b);
    Double_t FoundMeanLog(std::vector<float>* a,float b);

  private:
    //TH1F* fNClusters;
    //TH1F* fNHitInCluster;
      //TH1F* fClusterNW;
      //TH1F* fClusterNS;
    // Cosmic Rays
     //TH1F* fHitTime; 
     //TH1F* fHitArea;
      //TH1F* fHitIntegral;
      //TH2D* fHitAreaTime;
      TH1F* purityvalues;
      //TH2D* h_basediff2;
      //TH1F* h_basediff;
      TH1F* h_rms;
      TH1F* fRun; 
      TH2D* fRunSub;
      TProfile2D* fRunSubPurity;
      TProfile2D* fRunSubPurity2;
      TProfile2D* fRunSubPurity3;



    std::string fHitsModuleLabel;
    std::string fClusterModuleLabel;
    std::string fVertexModuleLabel;
      std::string fDigitModuleLabel;
    short fPrintLevel;
    short moduleID;
    float fValoretaufcl; 
    
    std::ofstream outFile;
      	 
  }; // class ICARUSPurityDQM

}

//#endif 

namespace cluster{

  //--------------------------------------------------------------------
  ICARUSPurityDQM::ICARUSPurityDQM(fhicl::ParameterSet const& pset)
    : EDAnalyzer(pset)
    , fHitsModuleLabel      (pset.get< std::string > ("HitsModuleLabel")         )
    , fClusterModuleLabel   (pset.get< std::string > ("ClusterModuleLabel"))
    , fVertexModuleLabel    (pset.get< std::string > ("VertexModuleLabel")         )
    , fDigitModuleLabel    (pset.get< std::string > ("RawModuleLabel")         )
    , fPrintLevel           (pset.get< short >       ("PrintLevel"))
    , fValoretaufcl         (pset.get< float >       ("ValoreTauFCL"))
  {
  
    if(fPrintLevel == -1) {
      // encode the clustermodule label into an integer
      moduleID = 0;
      size_t found = fClusterModuleLabel.find("traj"); if(found != std::string::npos) moduleID = 1;
      found = fClusterModuleLabel.find("line"); if(found != std::string::npos) moduleID = 2;
      found = fClusterModuleLabel.find("fuzz"); if(found != std::string::npos) moduleID = 3;
      found = fClusterModuleLabel.find("pand"); if(found != std::string::npos) moduleID = 4;
      std::cout<<"fClusterModuleLabel "<<fClusterModuleLabel<<" ID "<<moduleID<<"\n";
      
      std::string fileName = fClusterModuleLabel + ".tru";
      outFile.open(fileName);
    }
  
  }
  
  //------------------------------------------------------------------
  ICARUSPurityDQM::~ICARUSPurityDQM()
  {
  
  }
  
  void ICARUSPurityDQM::beginJob()
  {
    
    // get access to the TFile service
    art::ServiceHandle<art::TFileService> tfs;
  
    //fNClusters=tfs->make<TH1F>("fNoClustersInEvent","Number of Clusters", 400,0 ,400);
    //fNHitInCluster = tfs->make<TH1F>("fNHitInCluster","NHitInCluster",1000,0,10000);
      //fHitArea = tfs->make<TH1F>("fHitArea","fHitArea",1000,0,22000);
      //fHitTime = tfs->make<TH1F>("fHitTime","fHitTime",1000,0,4100);
      //fHitIntegral = tfs->make<TH1F>("fHitIntegral","fHitIntegral",1000,0,22000);
      //fHitAreaTime = tfs->make<TH2D>("fHitAreaTime","fHitAreaTime",1000,0,22000,25,0,2500);
      //fClusterNW = tfs->make<TH1F>("fClusterNW","fClusterNW",1000,0,2000);
      //fClusterNS = tfs->make<TH1F>("fClusterNS","fClusterNS",1000,0,2500);
      purityvalues = tfs->make<TH1F>("purityvalues","purityvalues",20000,-10,10);
      //h_basediff = tfs->make<TH1F>("h_basediff","h_basediff",200,0,0);
      h_rms = tfs->make<TH1F>("h_rms","h_rms",2000,0,20);
      //h_basediff2 = tfs->make<TH2D>("h_basediff2","h_basediff2",200,0,0,200,0,0);
     fRun=tfs->make<TH1F>("fRun","Events per run", 4000,0.5 ,4000.5);
     fRunSub=tfs->make<TH2D>("fRunSub","Events per run", 4000,0.5 ,4000.5,50,0.5,50.5);
    fRunSubPurity=tfs->make<TProfile2D>("fRunSubPurity","Events per run", 4000,0.5 ,4000.5,50,0.5,50.5);
   fRunSubPurity2=tfs->make<TProfile2D>("fRunSubPurity2","Events per run", 4000,0.5 ,4000.5,50,0.5,50.5);
   fRunSubPurity3=tfs->make<TProfile2D>("fRunSubPurity3","Events per run", 4000,0.5 ,4000.5,50,0.5,50.5);

  }
  
  void ICARUSPurityDQM::endJob()
  {
    if(fPrintLevel == -1) outFile.close();
  }
  
  
 int ICARUSPurityDQM::Nothere(std::vector<int>* a, int b){
    int Cisono=3;
        for(int i=0; i<(int)a->size();i++)
        {
            if((*a)[i]==b)
            {
                Cisono=8;
            }
        }
        
        return Cisono;//se "ci sono" vale 8 allora l'intero b e' contenuto nel vettore a mentre se "ci sono" vale 3 a non contiene b.
    }

    
    int ICARUSPurityDQM::Notheref(std::vector<float>* a, float b){
        int Cisono=3;
        for(int i=0; i<(int)a->size();i++)
        {
            if((*a)[i]==b)
            {
                Cisono=8;
            }
        }
        
        return Cisono;//se "ci sono" vale 8 allora l'intero b e' contenuto nel vettore a mentre se "ci sono" vale 3 a non contiene b.
    }
    
    Double_t ICARUSPurityDQM::FoundMeanLog(std::vector<float>* a,float b){
        
        int punto_taglio=a->size()*(1-b)+0.5;
        /////std::cout << punto_taglio << " e " << a->size() << std::endl;
        //if(punto_taglio==0)punto_taglio=1;
        std::vector<float>* usedhere=new std::vector<float>;
        for(int jj=0;jj<punto_taglio+1;jj++)
        {
            //cout << jj << endl;
            float maximum=0;
            for(int j=0;j<(int)a->size();j++)
            {
                if((*a)[j]>maximum && Notheref(usedhere,(*a)[j])==3)
                {
                    maximum=(*a)[j];
                }
            }
            //cout << maximum << endl;
            usedhere->push_back(maximum);
        }
        //cout << (*usedhere)[punto_taglio] << endl;
        return (*usedhere)[punto_taglio-1];
    }
    
    
    
    
  void ICARUSPurityDQM::analyze(const art::Event& evt)
  {
    std::cout << " Inizia Purity ICARUS Ana " << std::endl;
    // code stolen from TrackAna_module.cc
    art::ServiceHandle<geo::Geometry>      geom;
      unsigned int  fDataSize;
      std::vector<short> rawadc;      //UNCOMPRESSED ADC VALUES.
    // get all hits in the event
    //InputTag cluster_tag { "fuzzycluster" }; //CH comment trovato con eventdump code

    //to get run and event info, you use this "eventAuxillary()" object.
    art::Timestamp ts = evt.time();
    std::cout << "Processing for Purity " << " Run " << evt.run() << ", " << "Event " << evt.event() << " and Time " << ts.value() << std::endl;
	fRun->Fill(evt.run());
        fRunSub->Fill(evt.run(),evt.subRun());
      art::Handle< std::vector<raw::RawDigit> > digitVecHandle;
      evt.getByLabel(fDigitModuleLabel, digitVecHandle);
      std::vector<const raw::RawDigit*> rawDigitVec;

      if (digitVecHandle.isValid())
      {
          //unsigned int maxChannels    = fGeometry->Nchannels();
          //unsigned int maxTimeSamples = fDetectorProperties->NumberTimeSamples();
          // Sadly, the RawDigits come to us in an unsorted condition which is not optimal for
          // what we want to do here. So we make a vector of pointers to the input raw digits and sort them
          // Ugliness to fill the pointer vector...
          for(size_t idx = 0; idx < digitVecHandle->size(); idx++) rawDigitVec.push_back(&digitVecHandle->at(idx)); //art::Ptr<raw::RawDigit>(digitVecHandle, idx).get());
          // Sort (use a lambda to sort by channel id)
          std::sort(rawDigitVec.begin(),rawDigitVec.end(),[](const raw::RawDigit* left, const raw::RawDigit* right) {return left->Channel() < right->Channel();});
      }
 
      
       std::vector<int> *www0=new std::vector<int>;
       std::vector<float> *sss0=new std::vector<float>;
       std::vector<float> *hhh0=new std::vector<float>;
       std::vector<float> *ehh0=new std::vector<float>;
      std::vector<int> *www1=new std::vector<int>;
      std::vector<float> *sss1=new std::vector<float>;
      std::vector<float> *hhh1=new std::vector<float>;
      std::vector<float> *ehh1=new std::vector<float>;
      std::vector<int> *www2=new std::vector<int>;
      std::vector<float> *sss2=new std::vector<float>;
      std::vector<float> *hhh2=new std::vector<float>;
      std::vector<float> *ehh2=new std::vector<float>;
      std::vector<int> *www3=new std::vector<int>;
      std::vector<float> *sss3=new std::vector<float>;
      std::vector<float> *hhh3=new std::vector<float>;
      std::vector<float> *ehh3=new std::vector<float>;
      std::vector<int> *ccc0=new std::vector<int>;
      std::vector<int> *ccc1=new std::vector<int>;
      std::vector<int> *ccc2=new std::vector<int>;
      std::vector<int> *ccc3=new std::vector<int>;
      std::vector<float> *aaa0=new std::vector<float>;
      std::vector<float> *aaa1=new std::vector<float>;
      std::vector<float> *aaa2=new std::vector<float>;
      std::vector<float> *aaa3=new std::vector<float>;


      
      for(const auto& rawDigit : rawDigitVec)
      {
          raw::ChannelID_t channel = rawDigit->Channel();
          //std::cout << channel << std::endl;
          std::vector<geo::WireID> wids = geom->ChannelToWire(channel);
          // for now, just take the first option returned from ChannelToWire
          geo::WireID wid  = wids[0];
          // We need to know the plane to look up parameters
          geo::PlaneID::PlaneID_t plane = wid.Plane;
          size_t cryostat=wid.Cryostat;
          size_t tpc=wid.TPC;
          size_t iWire=wid.Wire;
          //std::cout << plane << " " << tpc << " " << cryostat << " " << iWire << std::endl;
          fDataSize = rawDigit->Samples();
          rawadc.resize(fDataSize);
          

          //UNCOMPRESS THE DATA.
          int pedestal = (int)rawDigit->GetPedestal();
          float pedestal2 = rawDigit->GetPedestal();
          float sigma_pedestal = rawDigit->GetSigma();
          raw::Uncompress(rawDigit->ADCs(), rawadc, pedestal, rawDigit->Compression());
          //std::cout << pedestal2 << " " << fDataSize << " " << rawadc.size() << " " << sigma_pedestal << std::endl;
          float massimo=0;
          float quale_sample_massimo;

          if (plane==2) {
              TH1F *h111 = new TH1F("h111","delta aree",20000,0,0);
              for (unsigned int ijk=0; ijk<(fDataSize); ijk++)
                {
                    //h111->Fill(rawDigit->ADC(ijk)-pedestal2);
                    //std::cout << rawDigit->ADC(ijk) << " " << rawDigit->ADC(ijk)-pedestal2 << std::endl;
                    if ((rawDigit->ADC(ijk)-pedestal2)>massimo && ijk>150 && ijk<(fDataSize-150))
                        {
                            massimo=(rawDigit->ADC(ijk)-pedestal2);
                            quale_sample_massimo=ijk;
                        }
                }
              //sigma_pedestal=h111->GetRMS();
              //h111->Delete();
              float base_massimo_before=0;
              float base_massimo_after=0;
              for (unsigned int ijk=quale_sample_massimo-150; ijk<quale_sample_massimo-50; ijk++)
              {
                  base_massimo_before+=(rawDigit->ADC(ijk)-pedestal2)*0.01;
              }
              for (unsigned int ijk=quale_sample_massimo+50; ijk<quale_sample_massimo+150; ijk++)
              {
                  base_massimo_after+=(rawDigit->ADC(ijk)-pedestal2)*0.01;
              }
              float basebase=(base_massimo_after+base_massimo_before)*0.5;
              float areaarea=0;

              for (unsigned int ijk=0; ijk<(fDataSize); ijk++)
              {
                  if (ijk>(quale_sample_massimo-30) && ijk<(quale_sample_massimo+30)) {
                  areaarea+=(rawDigit->ADC(ijk)-pedestal2-basebase);
                  }
                  else{
                      h111->Fill(rawDigit->ADC(ijk)-pedestal2);
                  }
                  
              }
              sigma_pedestal=h111->GetRMS();
              h111->Delete();
              //std::cout << "MASSIMO BASE " << massimo << " " << base_massimo_before << " " << base_massimo_after << std::endl;
              //h_basediff->Fill(fabs(base_massimo_after-base_massimo_before));
              //h_basediff2->Fill(fabs(base_massimo_after-base_massimo_before),sigma_pedestal);
              h_rms->Fill(sigma_pedestal);
              if (massimo>(5*sigma_pedestal) && fabs(base_massimo_after-base_massimo_before)<sigma_pedestal)
                {
                    
                    if(cryostat==0 && tpc==0)www0->push_back(iWire);
                    if(cryostat==0 && tpc==0)sss0->push_back(quale_sample_massimo);
                    if(cryostat==0 && tpc==0)hhh0->push_back(massimo);
                    if(cryostat==0 && tpc==0)ehh0->push_back(sigma_pedestal);
                    if(cryostat==0 && tpc==0)ccc0->push_back(-1);
                    if(cryostat==0 && tpc==1)www1->push_back(iWire);
                    if(cryostat==0 && tpc==1)sss1->push_back(quale_sample_massimo);
                    if(cryostat==0 && tpc==1)hhh1->push_back(massimo);
                    if(cryostat==0 && tpc==1)ehh1->push_back(sigma_pedestal);
                    if(cryostat==0 && tpc==1)ccc1->push_back(-1);
                    if(cryostat==1 && tpc==0)www2->push_back(iWire);
                    if(cryostat==1 && tpc==0)sss2->push_back(quale_sample_massimo);
                    if(cryostat==1 && tpc==0)hhh2->push_back(massimo);
                    if(cryostat==1 && tpc==0)ehh2->push_back(sigma_pedestal);
                    if(cryostat==1 && tpc==0)ccc2->push_back(-1);
                    if(cryostat==1 && tpc==1)www3->push_back(iWire);
                    if(cryostat==1 && tpc==1)sss3->push_back(quale_sample_massimo);
                    if(cryostat==1 && tpc==1)hhh3->push_back(massimo);
                    if(cryostat==1 && tpc==1)ehh3->push_back(sigma_pedestal);
                    if(cryostat==1 && tpc==1)ccc3->push_back(-1);
                    if(cryostat==0 && tpc==0)aaa0->push_back(areaarea);
                    if(cryostat==0 && tpc==1)aaa1->push_back(areaarea);
                    if(cryostat==1 && tpc==0)aaa2->push_back(areaarea);
                    if(cryostat==1 && tpc==1)aaa3->push_back(areaarea);
                    //std::cout << "MASSIMO " << massimo << " " << quale_sample_massimo << std::endl;
                    //std::cout << plane << " " << tpc << " " << cryostat << " " << iWire << std::endl;
                }
            }
        }
          
      for (unsigned int ijk=0; ijk<www0->size(); ijk++)
      {
          for (unsigned int ijk2=0; ijk2<www0->size(); ijk2++)
          {
              if (fabs((*www0)[ijk]-(*www0)[ijk2])<5 && ijk!=ijk2 && fabs((*sss0)[ijk]-(*sss0)[ijk2])<150)
              {
                  if ((*ccc0)[ijk]<0 && (*ccc0)[ijk2]<0)
                  {
                      (*ccc0)[ijk]=ijk+1;
                      (*ccc0)[ijk2]=ijk+1;
                  }
                  else if ((*ccc0)[ijk]>0 && (*ccc0)[ijk2]<0)
                  {
                      (*ccc0)[ijk2]=(*ccc0)[ijk];
                  }
                  else if ((*ccc0)[ijk]<0 && (*ccc0)[ijk2]>0)
                  {
                      (*ccc0)[ijk]=(*ccc0)[ijk2];
                  }
                  else if ((*ccc0)[ijk]>0 && (*ccc0)[ijk2]>0)
                  {
                      for (unsigned int ijk3=0; ijk3<www0->size(); ijk3++)
                      {
                          if(((*ccc0)[ijk3])==((*ccc0)[ijk2]))(*ccc0)[ijk3]=(*ccc0)[ijk];
                      }
                  }
              }
          }
      }
 
      for (unsigned int ijk=0; ijk<www1->size(); ijk++)
      {
          for (unsigned int ijk2=0; ijk2<www1->size(); ijk2++)
          {
              if (fabs((*www1)[ijk]-(*www1)[ijk2])<5 && ijk!=ijk2 && fabs((*sss1)[ijk]-(*sss1)[ijk2])<150)
              {
                  if ((*ccc1)[ijk]<0 && (*ccc1)[ijk2]<0)
                  {
                      (*ccc1)[ijk]=ijk+1;
                      (*ccc1)[ijk2]=ijk+1;
                  }
                  else if ((*ccc1)[ijk]>0 && (*ccc1)[ijk2]<0)
                  {
                      (*ccc1)[ijk2]=(*ccc1)[ijk];
                  }
                  else if ((*ccc1)[ijk]<0 && (*ccc1)[ijk2]>0)
                  {
                      (*ccc1)[ijk]=(*ccc1)[ijk2];
                  }
                  else if ((*ccc1)[ijk]>0 && (*ccc1)[ijk2]>0)
                  {
                      for (unsigned int ijk3=0; ijk3<www1->size(); ijk3++)
                      {
                          if(((*ccc1)[ijk3])==((*ccc1)[ijk2]))(*ccc1)[ijk3]=(*ccc1)[ijk];
                      }
                  }
              }
          }
      }

      for (unsigned int ijk=0; ijk<www2->size(); ijk++)
      {
          for (unsigned int ijk2=0; ijk2<www2->size(); ijk2++)
          {
              if (fabs((*www2)[ijk]-(*www2)[ijk2])<5 && ijk!=ijk2 && fabs((*sss2)[ijk]-(*sss2)[ijk2])<150)
              {
                  if ((*ccc2)[ijk]<0 && (*ccc2)[ijk2]<0)
                  {
                      (*ccc2)[ijk]=ijk+1;
                      (*ccc2)[ijk2]=ijk+1;
                  }
                  else if ((*ccc2)[ijk]>0 && (*ccc2)[ijk2]<0)
                  {
                      (*ccc2)[ijk2]=(*ccc2)[ijk];
                  }
                  else if ((*ccc2)[ijk]<0 && (*ccc2)[ijk2]>0)
                  {
                      (*ccc2)[ijk]=(*ccc2)[ijk2];
                  }
                  else if ((*ccc2)[ijk]>0 && (*ccc2)[ijk2]>0)
                  {
                      for (unsigned int ijk3=0; ijk3<www2->size(); ijk3++)
                      {
                          if(((*ccc2)[ijk3])==((*ccc2)[ijk2]))(*ccc2)[ijk3]=(*ccc2)[ijk];
                      }
                  }
              }
          }
      }

      for (unsigned int ijk=0; ijk<www3->size(); ijk++)
      {
          for (unsigned int ijk2=0; ijk2<www3->size(); ijk2++)
          {
              if (fabs((*www3)[ijk]-(*www3)[ijk2])<5 && ijk!=ijk2 && fabs((*sss3)[ijk]-(*sss3)[ijk2])<150)
              {
                  if ((*ccc3)[ijk]<0 && (*ccc3)[ijk2]<0)
                  {
                      (*ccc3)[ijk]=ijk+1;
                      (*ccc3)[ijk2]=ijk+1;
                  }
                  else if ((*ccc3)[ijk]>0 && (*ccc3)[ijk2]<0)
                  {
                      (*ccc3)[ijk2]=(*ccc3)[ijk];
                  }
                  else if ((*ccc3)[ijk]<0 && (*ccc3)[ijk2]>0)
                  {
                      (*ccc3)[ijk]=(*ccc3)[ijk2];
                  }
                  else if ((*ccc3)[ijk]>0 && (*ccc3)[ijk2]>0)
                  {
                      for (unsigned int ijk3=0; ijk3<www3->size(); ijk3++)
                      {
                          if(((*ccc3)[ijk3])==((*ccc3)[ijk2]))(*ccc3)[ijk3]=(*ccc3)[ijk];
                      }
                  }
              }
          }
      }

      
      
      
      Int_t clusters_creation[4][6000];
      //Int_t clusters_avewire[4][1000];
      Int_t clusters_swire[4][6000];
      Int_t clusters_lwire[4][6000];
      Int_t clusters_ssample[4][6000];
      Int_t clusters_lsample[4][6000];
      
      Int_t clusters_nn[1000];
      Int_t clusters_vi[1000];
      Int_t clusters_qq[1000];
      //Int_t clusters_avewire[4][1000];
      Int_t clusters_dw[1000];
      Int_t clusters_ds[1000];


      for (unsigned int ijk=0; ijk<4; ijk++) {
          for (unsigned int ijk2=0; ijk2<6000; ijk2++) {
              clusters_creation[ijk][ijk2]=0;
              clusters_swire[ijk][ijk2]=100000;
              clusters_lwire[ijk][ijk2]=0;
              clusters_ssample[ijk][ijk2]=100000;
              clusters_lsample[ijk][ijk2]=0;
              if (ijk2<1000 && ijk>2) {
                  clusters_nn[ijk2]=-1;
                  clusters_vi[ijk2]=-1;
                  clusters_dw[ijk2]=-1;
                  clusters_ds[ijk2]=-1;
                  clusters_qq[ijk2]=-1;
              }
          }
      }
      
      for (unsigned int ijk=0; ijk<www0->size(); ijk++)
      {
          if ((*ccc0)[ijk]>0) {
              int numero=(*ccc0)[ijk];
              clusters_creation[0][numero]+=1;
              if((*www0)[ijk]<clusters_swire[0][numero])clusters_swire[0][numero]=(*www0)[ijk];
              if((*www0)[ijk]>clusters_lwire[0][numero])clusters_lwire[0][numero]=(*www0)[ijk];
              if((*sss0)[ijk]<clusters_ssample[0][numero])clusters_ssample[0][numero]=(*sss0)[ijk];
              if((*sss0)[ijk]>clusters_lsample[0][numero])clusters_lsample[0][numero]=(*sss0)[ijk];
          }
      }
    for (unsigned int ijk=0; ijk<www1->size(); ijk++)
    {
          if ((*ccc1)[ijk]>0) {
              int numero=(*ccc1)[ijk];
              clusters_creation[1][numero]+=1;
              if((*www1)[ijk]<clusters_swire[1][numero])clusters_swire[1][numero]=(*www1)[ijk];
              if((*www1)[ijk]>clusters_lwire[1][numero])clusters_lwire[1][numero]=(*www1)[ijk];
              if((*sss1)[ijk]<clusters_ssample[1][numero])clusters_ssample[1][numero]=(*sss1)[ijk];
              if((*sss1)[ijk]>clusters_lsample[1][numero])clusters_lsample[1][numero]=(*sss1)[ijk];
          }
    }
    for (unsigned int ijk=0; ijk<www2->size(); ijk++)
    {
          if ((*ccc2)[ijk]>0) {
              int numero=(*ccc2)[ijk];
              clusters_creation[2][numero]+=1;
              if((*www2)[ijk]<clusters_swire[2][numero])clusters_swire[2][numero]=(*www2)[ijk];
              if((*www2)[ijk]>clusters_lwire[2][numero])clusters_lwire[2][numero]=(*www2)[ijk];
              if((*sss2)[ijk]<clusters_ssample[2][numero])clusters_ssample[2][numero]=(*sss2)[ijk];
              if((*sss2)[ijk]>clusters_lsample[2][numero])clusters_lsample[2][numero]=(*sss2)[ijk];
          }
    }
    for (unsigned int ijk=0; ijk<www3->size(); ijk++)
    {
          if ((*ccc3)[ijk]>0) {
              int numero=(*ccc3)[ijk];
              clusters_creation[3][numero]+=1;
              if((*www3)[ijk]<clusters_swire[3][numero])clusters_swire[3][numero]=(*www3)[ijk];
              if((*www3)[ijk]>clusters_lwire[3][numero])clusters_lwire[3][numero]=(*www3)[ijk];
              if((*sss3)[ijk]<clusters_ssample[3][numero])clusters_ssample[3][numero]=(*sss3)[ijk];
              if((*sss3)[ijk]>clusters_lsample[3][numero])clusters_lsample[3][numero]=(*sss3)[ijk];
          }
    }

      int quanti_clusters=0;
      for (unsigned int ijk=0; ijk<4; ijk++) {
          for (unsigned int ijk2=0; ijk2<6000; ijk2++) {
              if(clusters_creation[ijk][ijk2]>50)
              {
                  std::cout<<clusters_creation[ijk][ijk2] << " CLUSTER MINE " << clusters_swire[ijk][ijk2] << " " << clusters_lwire[ijk][ijk2] << " " << clusters_ssample[ijk][ijk2] << " " << clusters_lsample[ijk][ijk2] << std::endl;
                  clusters_qq[quanti_clusters]=clusters_creation[ijk][ijk2];
                  clusters_vi[quanti_clusters]=ijk;
                  clusters_nn[quanti_clusters]=ijk2;
                  clusters_dw[quanti_clusters]=clusters_lwire[ijk][ijk2]-clusters_swire[ijk][ijk2];
                  clusters_ds[quanti_clusters]=clusters_lsample[ijk][ijk2]-clusters_ssample[ijk][ijk2];
                  quanti_clusters+=1;
              }
          }
      }

      
    for(int icl = 0; icl < quanti_clusters; ++icl){
      if (clusters_qq[icl]>0) {
	std::cout << " CLUSTER INFO " << icl << " " << clusters_qq[icl] << " " << clusters_vi[icl] << " " << clusters_dw[icl] << " " << clusters_ds[icl] << " " << clusters_nn[icl] << std::endl;
	int tpc_number=clusters_vi[icl];//qui andrebbe messo il numero di TPC
        
	if (clusters_ds[icl]>1000 && clusters_dw[icl]>100)
	  {//if analisi

	    std::vector<float> *whc=new std::vector<float>;
	    std::vector<float> *shc=new std::vector<float>;
	    std::vector<float> *ahc=new std::vector<float>;
	    std::vector<float> *fahc=new std::vector<float>;
 
          if (clusters_vi[icl]==0) {
              for (unsigned int ijk=0; ijk<www0->size(); ijk++) {
                  if ((*ccc0)[ijk]==clusters_nn[icl]) {
                      whc->push_back((*www0)[ijk]);
                      shc->push_back((*sss0)[ijk]);
                      ///ahc->push_back((*hhh0)[ijk]);
                      ahc->push_back((*aaa0)[ijk]);
                      fahc->push_back((*ehh0)[ijk]);
                  }
              }
          }
          if (clusters_vi[icl]==1) {
              for (unsigned int ijk=0; ijk<www1->size(); ijk++) {
                  if ((*ccc1)[ijk]==clusters_nn[icl]) {
                      whc->push_back((*www1)[ijk]);
                      shc->push_back((*sss1)[ijk]);
                      ahc->push_back((*aaa1)[ijk]);
                      //ahc->push_back((*hhh1)[ijk]);
                      fahc->push_back((*ehh1)[ijk]);
                  }
              }
          }
          if (clusters_vi[icl]==2) {
              for (unsigned int ijk=0; ijk<www2->size(); ijk++) {
                  if ((*ccc2)[ijk]==clusters_nn[icl]) {
                      whc->push_back((*www2)[ijk]);
                      shc->push_back((*sss2)[ijk]);
                      ahc->push_back((*aaa2)[ijk]);
                      //ahc->push_back((*hhh2)[ijk]);
                      fahc->push_back((*ehh2)[ijk]);
                  }
              }
          }
          if (clusters_vi[icl]==3) {
              for (unsigned int ijk=0; ijk<www3->size(); ijk++) {
                  if ((*ccc3)[ijk]==clusters_nn[icl]) {
                      whc->push_back((*www3)[ijk]);
                      shc->push_back((*sss3)[ijk]);
                      //ahc->push_back((*hhh3)[ijk]);
                      ahc->push_back((*aaa3)[ijk]);
                      fahc->push_back((*ehh3)[ijk]);
                  }
              }
          }

          
          std::cout << " CLUSTER INFO " << icl << " " << clusters_qq[icl] << " " << whc->size() << std::endl;

	    
	    if(whc->size()>30)//prima 0
	      {
		float pendenza=0;float intercetta=0;int found_ok=0;
		std::vector<int> *escluse=new std::vector<int>;
		for(int j=0;j<(int)whc->size();j++)
		  {
		    if(found_ok<1)
		      {
			Double_t wires[10000];
			Double_t samples[10000];
			Double_t ex[10000];
			Double_t quale[10000];
			Double_t ey[10000];
			int quanti=0;
			for(int k=0;k<(int)whc->size();k++)
			  {
			    if(Nothere(escluse,k)==3)
			      {
				wires[quanti]=(*whc)[k]*3;
				samples[quanti]=(*shc)[k]*0.628;
				ex[quanti]=0;
				ey[quanti]=0;
				quale[quanti]=k;
				quanti+=1;
			      }
			  }
			//////std::cout << quanti << std::endl;
			TGraphErrors *gr3 = new TGraphErrors(quanti,wires,samples,ex,ey);
			gr3->Fit("pol1");
                        TF1 *fitfunc = gr3->GetFunction("pol1");
                        pendenza=fitfunc->GetParameter(1);
                        intercetta=fitfunc->GetParameter(0);
                        float distance_maximal=3;
                        int quella_a_distance_maximal=0;
                        int found_max=0;
                        for(int jj=0;jj<quanti;jj++)
			  {
                            if((abs((pendenza)*(wires[jj])-samples[jj]+intercetta)/sqrt((pendenza)*pendenza+1))>distance_maximal)
			      {
				found_max=1;
				quella_a_distance_maximal=quale[jj];
				distance_maximal=(abs(pendenza*wires[jj]-samples[jj]+intercetta)/sqrt(pendenza*pendenza+1));
			      }
			  }
			if(found_max==1)escluse->push_back(quella_a_distance_maximal);
			if(found_max==0)found_ok=1;
		      }
		  }
                std::cout << escluse->size() << " escluse " << whc->size() << " " << found_ok << std::endl;
                delete escluse;
                std::vector<float> *hittime=new std::vector<float>;
                std::vector<float> *hitwire=new std::vector<float>;
                std::vector<float> *hitarea=new std::vector<float>;
                std::vector<float> *hittimegood=new std::vector<float>;
                std::vector<float> *hitareagood=new std::vector<float>;
                std::vector<float> *hitwiregood=new std::vector<float>;
                /////std::cout <<  found_ok << std::endl;
                /////std::cout <<  pendenza << std::endl;
                /////std::cout <<  intercetta << std::endl;
                if(found_ok==1)
		  {
                    //float t_max=-1400/0.4*log(8/(3.3*peach));
                    //float t_max=3800;
                    //if(t_max>1500) t_max=1500;
                    for(int kkk=0;kkk<(int)whc->size();kkk++)
		      {
                        if((*ahc)[kkk]>0)
			  {
                            float distance=(abs(pendenza*((*whc)[kkk]*3)-(*shc)[kkk]*0.628+intercetta))/sqrt(pendenza*pendenza+1);
                            if(distance<=3)
			      {
                                //cout << log((*ahc)[kkk]/(0.4*peach)) << endl;
                                hittime->push_back((*shc)[kkk]*0.4);
                                hitwire->push_back((*whc)[kkk]);
                                hitarea->push_back((*ahc)[kkk]);
			      }
			  }
		      }
                    //float result_rms=0.14;
                    //std::cout << result_rms << endl;
                    Double_t area[10000];
                    Double_t nologarea[10000];
                    Double_t tempo[10000];
                    Double_t ex[10000];
                    //Double_t quale[10000];
                    Double_t ey[10000];
                    Double_t ek[10000];
                    Double_t ez[10000];
                    std::cout <<  hitarea->size() << " dimensione hitarea" << std::endl;
                    if(hitarea->size()>100)//prima 30
		      {
                        float minimo=100000;
                        float massimo=0;
                        for(int kk=0;kk<(int)hitarea->size();kk++)
                        {
			  if((*hittime)[kk]>massimo)massimo=(*hittime)[kk];
			  if((*hittime)[kk]<minimo)minimo=(*hittime)[kk];
                        }
                        //std::cout << hitarea->size() << std::endl;
                        //int gruppi=hitarea->size()/50;
                        int gruppi=8;
                        //std::cout << gruppi << std::endl;
                        float steptime=(massimo-minimo)/(gruppi+1);
                        //std::cout << steptime << " steptime " << minimo << " " << massimo << std::endl;
                        float starting_value_tau=fValoretaufcl;
                        ////////std::cout << starting_value_tau << " VALORE INDICATIVO TAU " << std::endl;
                        //if(tpc_number==2 || tpc_number==5)starting_value_tau=6500;
                        //if(tpc_number==10 || tpc_number==13)starting_value_tau=5700;
                        for(int stp=0;stp<=gruppi;stp++)
			  {
			    std::vector<float>* hitpertaglio=new std::vector<float>;
                            //std::cout << 500+stp*steptime << " time " << 500+(stp+1)*(steptime) << std::endl;
                            ///////std::cout << minimo+stp*steptime << " " << minimo+(stp+1)*(steptime) << std::endl;
                            for(int kk=0;kk<(int)hitarea->size();kk++)
			      {
                                if((*hittime)[kk]>=(minimo+stp*steptime) && (*hittime)[kk]<=(minimo+(stp+1)*(steptime))) hitpertaglio->push_back((*hitarea)[kk]*exp((*hittime)[kk]/starting_value_tau));
			      }
                            ///////std::cout << hitpertaglio->size() << std::endl;
                            float tagliomax=FoundMeanLog(hitpertaglio,0.90);
                            float tagliomin=FoundMeanLog(hitpertaglio,0.10);
                            //float tagliomin=0;
                            //float tagliomax=1000000;
                            delete hitpertaglio;
                            //std::cout << tagliomax << " t " << std::endl;
                            for(int kk=0;kk<(int)hitarea->size();kk++)
			      {
                                //std::cout << (*hittime)[kk] << " " << (*hitwire)[kk] << " " << (*hitarea)[kk] << " " << (minimo+stp*steptime) << " " << (minimo+(stp+1)*steptime) << " " << (*hitarea)[kk]*exp((*hittime)[kk]/starting_value_tau) << std::endl;
                                if((*hittime)[kk]>(minimo+stp*steptime) && (*hittime)[kk]<(minimo+(stp+1)*steptime) && (*hitarea)[kk]*exp((*hittime)[kk]/starting_value_tau)<tagliomax && (*hitarea)[kk]*exp((*hittime)[kk]/starting_value_tau)>tagliomin)
				  {
                                    //std::cout << ((*hitarea)[kk]*exp((*hittime)[kk]/1400)) << " GOOD " << (*hitarea)[kk] << " " << (*hittime)[kk] << std::endl;
                                    hitareagood->push_back((*hitarea)[kk]);
                                    hittimegood->push_back((*hittime)[kk]);
                                    hitwiregood->push_back((*hitwire)[kk]);
				  }
			      }
			  }
                    std::cout << hitareagood->size() << " hitareagood" << std::endl;    
                        for(int k=0;k<(int)hitareagood->size();k++)
			  {
                            //if((*hittimegood)[k]-600*0.4<=1000)//correzione 15/08
                            if((*hittimegood)[k]<=2240)
			      {
                                tempo[k]=(*hittimegood)[k];
                                area[k]=log((*hitareagood)[k]);
                                //std::cout << (*hitareagood)[k] << " " << area[k] << std::endl;
                                nologarea[k]=((*hitareagood)[k]);
                                ex[k]=0;
                                ez[k]=60;
                                ey[k]=0.23;
			      }
			  }
                        TGraphErrors *gr31 = new TGraphErrors(hitareagood->size(),tempo,area,ex,ey);
                        //TGraphErrors *gr4 = new TGraphErrors(hitareagood->size(),tempo,nologarea,ex,ey);
                        gr31->Fit("pol1");
                        TF1 *fit = gr31->GetFunction("pol1");
                        float slope_purity=fit->GetParameter(1);
                        //float error_slope_purity=fit->GetParError(1);
                        float intercetta_purezza=fit->GetParameter(0);
                  
                        TH1F *h111 = new TH1F("h111","delta aree",200,-10,10);
                        float sum_per_rms_test=0;
                        for(int k=0;k<(int)hitareagood->size();k++)
			  {
                            h111->Fill(area[k]-slope_purity*tempo[k]-intercetta_purezza);
                            sum_per_rms_test+=(area[k]-slope_purity*tempo[k]-intercetta_purezza)*(area[k]-slope_purity*tempo[k]-intercetta_purezza);
			  }
                        h111->Fit("gaus");
                        TF1 *fitg = h111->GetFunction("gaus");
                        float error=fitg->GetParameter(2);
                        std::cout << " error " << error << std::endl;
                        float error_2=sqrt(sum_per_rms_test/(hitareagood->size()-2));
                        std::cout << " error vero" << error_2 << std::endl;
                        h111->Delete();
                  
                        for(int k=0;k<(int)hitareagood->size();k++)
			  {
                            //if((*hittimegood)[k]-600*0.4<=1000)//15/08
                            if((*hittimegood)[k]<=2240)
			      {
                                ek[k]=-nologarea[k]+exp(area[k]+error);
                                ez[k]=nologarea[k]-exp(area[k]-error);
                                ey[k]=error;
			      }
			  }
                        
                        TGraphErrors *gr32 = new TGraphErrors(hitareagood->size(),tempo,area,ex,ey);
                        gr32->Fit("pol1");
                        TCanvas c4 ("c4", "coll 1 ind 2", 10, 10, 700, 700);
                        c4.Range(-79.46542,0.7467412,586.8952,3.249534);
                        c4.SetFillColor(0);
                        c4.SetBorderSize(2);
                        c4.SetLeftMargin(0.1192529);
                        c4.SetRightMargin(0.08045977);
                        c4.SetFrameLineWidth(2);
                        gr32->Draw("AP");
                        c4.Print("grafico2.eps");
                        c4.Print("grafico2.C");
                        
                        TF1 *fit2 = gr32->GetFunction("pol1");
                        float slope_purity_2=fit2->GetParameter(1);
                        float error_slope_purity_2=fit2->GetParError(1);
                        //float intercetta_purezza_2=fit2->GetParameter(0);
                        //float chiquadro=fit2->GetChisquare()/(hitareagood->size()-2);
			std::ofstream goodpuro("purity_results.out",std::ios::app);
			purityvalues->Fill(-slope_purity_2*1000.);
			fRunSubPurity->Fill(evt.run(),evt.subRun(),-slope_purity_2*1000.);
                        /////if(tpc_number==2 || tpc_number==5)
			/////  {
                            //goodpuro << " run " << evt.Run() << " event " << evt.event() << " vista " << tpc_number << std::endl;
                            //goodpuro << chiquadro << std::endl;
                            goodpuro << evt.run() << " " << evt.subRun() << " " << evt.event() << " " << tpc_number << " " << slope_purity_2 << " " << error_slope_purity_2 << std::endl;
			  //////}
                        std::cout << -1/slope_purity_2 << std::endl;
                        std::cout << -1/(slope_purity_2+error_slope_purity_2)+1/slope_purity_2 << std::endl;
                        std::cout << 1/slope_purity_2-1/(slope_purity_2-error_slope_purity_2) << std::endl;
                        TGraphAsymmErrors *gr41 = new TGraphAsymmErrors (hitareagood->size(),tempo,nologarea,ex,ex,ez,ek);
                        gr41->Fit("expo");
                        TF1 *fitexo = gr41->GetFunction("expo");
                        float slope_purity_exo=fitexo->GetParameter(1);
                        float error_slope_purity_exo=fitexo->GetParError(1);
                        fRunSubPurity2->Fill(evt.run(),evt.subRun(),-slope_purity_exo*1000.);
                        std::cout << -1/slope_purity_exo << std::endl;
                        std::cout << -1/(slope_purity_exo+error_slope_purity_exo)+1/slope_purity_exo << std::endl;
                        std::cout << 1/slope_purity_exo-1/(slope_purity_exo-error_slope_purity_exo) << std::endl;
                        std::cout << fitexo->GetChisquare()/(hitareagood->size()-2) << std::endl;
                        //std::cout << ts << " is time event " << std::endl;
                        //goodpur << -1/slope_purity_exo << std::endl;
                        //goodpur << -1/(slope_purity_exo+error_slope_purity_exo)+1/slope_purity_exo << std::endl;
                        //goodpur << 1/slope_purity_exo-1/(slope_purity_exo-error_slope_purity_exo) << std::endl;
                        //goodpur << timeevent << " is time event " << std::endl;
                       
                        TF1 *fitexo2 = new TF1("fitexo2","pol0+expo(1)");
			fitexo2->SetParLimits(0,-100,100); 
                        TGraphAsymmErrors *gr41b = new TGraphAsymmErrors (hitareagood->size(),tempo,nologarea,ex,ex,ez,ek);
			gr41b->Fit("fitexo2");
                        //gr41b->Fit("pol0(0)+expo(1)");
                        //TF1 *fitexo2 = gr41b->GetFunction("pol0(0)+expo(1)");
                        float slope_purity_exo2=fitexo2->GetParameter(2);
                        float error_slope_purity_exo2=fitexo2->GetParError(2);
                        fRunSubPurity3->Fill(evt.run(),evt.subRun(),-slope_purity_exo2*1000.);
                        std::cout << -1/slope_purity_exo2 << std::endl;
                        std::cout << -1/(slope_purity_exo2+error_slope_purity_exo2)+1/slope_purity_exo2 << std::endl;
                        std::cout << 1/slope_purity_exo2-1/(slope_purity_exo2-error_slope_purity_exo2) << std::endl;
                        std::cout << fitexo2->GetChisquare()/(hitareagood->size()-2) << std::endl;

			std::cout << -1/slope_purity_exo2 << " " << -1/slope_purity_exo << " TUTTTI I VALUES " << -slope_purity_exo2*1000. << " " << -slope_purity_exo*1000. << std::endl;

                        TCanvas c2 ("c2", "coll 1 ind 2", 10, 10, 700, 700);
                        c2.Range(-79.46542,0.7467412,586.8952,3.249534);
                        c2.SetFillColor(0);
                        c2.SetBorderSize(2);
                        c2.SetLogy();
                        c2.SetLeftMargin(0.1192529);
                        c2.SetRightMargin(0.08045977);
                        c2.SetFrameLineWidth(2);
                        //gr41->SetTitle("July, 10,2010, 17:45 #tau = 1473 #mus #pm 10%");
                        gr41b->SetTitle("Purity Measurement");
                        gr41b->SetFillColor(1);
                        gr41b->SetLineColor(4);
                        gr41b->SetLineWidth(2);
                        gr41b->Draw("AP");
                        gr41b->SetMinimum(50);
                        gr41b->SetMaximum(5000);
                        //gr41->SetDirectory(0);
                        //gr41->SetStats(0);
                        gr41b->GetXaxis()->SetTitle("hit time(#mus)");
                        gr41b->GetYaxis()->SetTitle("hit area(ADC# * tsample)");
                        gr41b->GetYaxis()->SetTitleOffset(1.3);
                        fitexo2->SetFillColor(19);
                        fitexo2->SetFillStyle(0);
                        fitexo2->SetLineWidth(3);
                        
                        c2.Print("grafico.eps");
                        c2.Print("grafico.C");
		      }//hitarea size 30
		  }
                delete hittime;
                delete hitarea;
                delete hittimegood;
                delete hitareagood;
                delete hitwiregood;
                delete hitwire;
		
	      
	      }//end whc->size>0
	    
            
	    delete shc;
	    delete ahc;
	    delete fahc;
	    delete whc;
	    
	  } 
 
      }//fine if ananlisi
    }
      delete www0;
      delete sss0;
      delete hhh0;
      delete ehh0;
      delete ccc0;
      delete www1;
      delete sss1;
      delete hhh1;
      delete ehh1;
      delete ccc1;
      delete www2;
      delete sss2;
      delete hhh2;
      delete ehh2;
      delete ccc2;
      delete www3;
      delete sss3;
      delete hhh3;
      delete ehh3;
      delete ccc3;
      delete aaa0;
      delete aaa1;
      delete aaa2;
      delete aaa3;

  } // analyze

} //end namespace


namespace cluster{

  DEFINE_ART_MODULE(ICARUSPurityDQM)
  
} 

