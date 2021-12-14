////////////////////////////////////////////////////////////////////////
// Class:       FragmentDQMAna
// Module Type: analyzer
// File:        FragmentDQMAna_module.cc
// Description: Looks at Fragments coming from DAQ, sends useful metrics
//              to DQM.
// Authors: Wes (wketchum@fnal.gov) and Moon (munjung@uchicago.edu)
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

//#include "sbndaq-artdaq-core/Overlays/Common/BernCRTFragment.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "sbndaq-artdaq-core/Overlays/FragmentType.hh"

//add these
#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
//---

//#include "art/Framework/Services/Optional/TFileService.h"

//#include "sbndaq-artdaq-core/Overlays/Common/BernCRTTranslator.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>
#include <unistd.h>


namespace sbndaq {
  class FragmentDQMAna;
}

/*****/

class sbndaq::FragmentDQMAna : public art::EDAnalyzer {

public:
  explicit FragmentDQMAna(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  virtual ~FragmentDQMAna();
  
  virtual void analyze(art::Event const & evt);
 
private:
  
};

//Define the constructor
sbndaq::FragmentDQMAna::FragmentDQMAna(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }
  //sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_channel_config"));
  //sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_board_config"));
}

sbndaq::FragmentDQMAna::~FragmentDQMAna()
{
}

void sbndaq::FragmentDQMAna::analyze(art::Event const & evt) {

  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;  
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();
  std::cout << std::endl;


  //loop over fragments in event
  //two different loop logics, depending on whether we have fragment containers or fragments
  auto fragmentHandles = evt.getMany<artdaq::Fragments>(); //returns std::vector< art::Handle< std::vector<artdaq::Fragment> > >

  std::cout << "We have " << fragmentHandles.size() << " fragment collections." << std::endl;

  for (auto const& handle : fragmentHandles) {

    //handle is art::Handle< std::vector<artdaq::Fragment> >

    if (!handle.isValid() || handle->size() == 0)
      continue;

    for (auto const& frag : *handle){
      //frag is artdaq::Fragment

      if(frag.type() != artdaq::Fragment::ContainerFragmentType)
	continue;

      artdaq::ContainerFragment cont_frag(frag);

      std::cout << "Container fragment type is " << (unsigned int)cont_frag.fragment_type() << std::endl;

    }//end loop over handle
  }//end loop over all handles

} //analyze


DEFINE_ART_MODULE(sbndaq::FragmentDQMAna) //this is where the module name is specified
