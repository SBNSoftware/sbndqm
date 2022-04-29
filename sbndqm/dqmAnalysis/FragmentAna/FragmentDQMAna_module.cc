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
bool verbose = false;

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

  int level = 0;

  unsigned int const fragid = frag.fragmentID();
  std::string fragment_id = std::to_string(fragid);

  artdaq::ContainerFragment cont_frag(frag);
  //get frag count
  uint64_t frag_count = cont_frag.block_count();
  //get zero rate
  uint64_t nzero = 0;
  if (frag_count == 0) { nzero = 1; }

  std::string group_name = "unknown_cont_frag";

  if (cont_frag.fragment_type() == sbndaq::detail::FragmentType::CAENV1730) {group_name = "PMT_cont_frag";}
  else if (cont_frag.fragment_type() == sbndaq::detail::FragmentType::BERNCRTV2) {group_name = "CRT_cont_frag";}

  sbndaq::sendMetric(group_name, fragment_id, "frag_count", frag_count, level, artdaq::MetricMode::Average);
  sbndaq::sendMetric(group_name, fragment_id, "zero_rate", nzero, level, artdaq::MetricMode::Rate);

  if (verbose) {std::cout << "sending: " << group_name << ":" << fragment_id << ", frag count: " << frag_count << ", nzero: " << nzero << std::endl;}

  }


//analyze
void sbndaq::FragmentDQMAna::analyze(art::Event const & evt) {

  if (verbose) {
    std::cout << "######################################################################" << std::endl;
    std::cout << std::endl;
    std::cout << "Run " << evt.run() << ", subrun " << evt.subRun()<< ", event " << evt.event();
    std::cout << std::endl;
  }

  //save fragment counts for the event in a vector
  std::vector<double> fragment_count;
  std::vector<double> fragment_ID;

  //loop over fragments in event
  auto fragmentHandles = evt.getMany<artdaq::Fragments>(); //returns std::vector< art::Handle< std::vector<artdaq::Fragment> > >

  if (verbose) {std::cout << "We have " << fragmentHandles.size() << " fragment collections." << std::endl;}

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

    }//end loop over handle
  }//end loop over all handles

} //analyze


DEFINE_ART_MODULE(sbndaq::FragmentDQMAna) //this is where the module name is specified                                                                                                                                   
