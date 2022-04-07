///////////////////////////////////////////////////////////////////////
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
  void analyze_fragment(artdaq::Fragment frag);

};

//Define the constructor
sbndaq::FragmentDQMAna::FragmentDQMAna(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
{

  if (pset.has_key("metrics")) {
    sbndaq::InitializeMetricManager(pset.get<fhicl::ParameterSet>("metrics"));
  }

  sbndaq::GenerateMetricConfig(pset.get<fhicl::ParameterSet>("metric_config"));
}

sbndaq::FragmentDQMAna::~FragmentDQMAna()
{
}

//analyze_fragment
void sbndaq::FragmentDQMAna::analyze_fragment(artdaq::Fragment frag) {

  artdaq::ContainerFragment cont_frag(frag);

  uint64_t frag_count = cont_frag.block_count();
  //get zero rate
  uint64_t nzero;
  if (frag_count == 0) { nzero = 1; }
  if (frag_count > 0) { nzero = 0; }

  unsigned int const fragid = (cont_frag[0].get() ->  fragmentID());
  std::string fragment_id = std::to_string(fragid);
  int level = 2;
  artdaq::MetricMode average = artdaq::MetricMode::Average;
  artdaq::MetricMode rate = artdaq::MetricMode::Rate;

  if (cont_frag.fragment_type() == 2) {
    std::string group_name = "PMT_cont_frag";

    sbndaq::sendMetric(group_name, fragment_id, "frag_count", frag_count, level, average);
    sbndaq::sendMetric(group_name, fragment_id, "zero_rate", nzero, level, rate);

    std::cout << "sending: " << group_name << ":" << fragment_id << ", frag count: " << frag_count << ", nzero: " << nzero << std::endl;

  }

  if (cont_frag.fragment_type() == 14) {
    std::string group_name = "CRT_cont_frag";

    sbndaq::sendMetric(group_name, fragment_id, "frag_count", frag_count, level, average);
    sbndaq::sendMetric(group_name, fragment_id, "zero_rate", nzero, level, rate);

    std::cout << "sending: " << group_name << ":" << fragment_id << ", frag count: " << frag_count << ", nzero: " << nzero << std::endl;

  }


}


//analyze
void sbndaq::FragmentDQMAna::analyze(art::Event const & evt) {

  std::cout << "######################################################################" << std::endl;
  std::cout << std::endl;
  std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();
  std::cout << std::endl;

  //save fragment counts for the event in a vector
  std::vector<double> fragment_count;
  std::vector<double> fragment_ID;

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

      // if fragment is a container fragment, print # of fragments in that container fragment
      if(frag.type() != artdaq::Fragment::ContainerFragmentType)
        continue;

      artdaq::ContainerFragment cont_frag(frag);
      analyze_fragment(frag);

      //std::cout << "Container fragment type is " << (unsigned int)cont_frag.fragment_type() << std::endl;
      //2:PMT, 14: CRT

      // all fragments in one container fragment have the same fragment ID
      //std::cout << "Container fragment ID is " << cont_frag[0].get() ->  fragmentID() << std::endl;
      //std::cout << "container fragment has " << (unsigned int)cont_frag.block_count() << " fragments" << std::endl;


      fragment_count.push_back((unsigned int)cont_frag.block_count());
      fragment_ID.push_back(cont_frag[0].get()->fragmentID());


    }//end loop over handle
  }//end loop over all handles

} //analyze


DEFINE_ART_MODULE(sbndaq::FragmentDQMAna) //this is where the module name is specified                                                                                                                                   
