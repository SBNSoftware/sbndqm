////////////////////////////////////////////////////////////////////////
// Class:       CAENV1730Stream
// Module Type: analyzer
// File:        CAENV1730Stream_module.cc
// Description: Saves information about each event to a MetricManager stream.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "sbndaq-artdaq-core/Overlays/Common/CAENV1730Fragment.hh"
#include "artdaq-core/Data/Fragment.hh"

#include "sbndaq-decode/Mode/Mode.hh"
#include "sbndaq-online/helpers/SBNMetricManager.h"

#include "TH1F.h"
#include "TNtuple.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>

namespace sbndaq {
  class CAENV1730Stream;
}

class sbndaq::CAENV1730Stream : public art::EDAnalyzer {

	public:
 	 explicit CAENV1730Stream(fhicl::ParameterSet const & pset); // explicit doesn't allow for copy initialization
  	 virtual ~CAENV1730Stream();

  	 virtual void analyze(art::Event const & evt);
        int16_t Baseline(const int16_t *data, size_t n_adc);
        double RMS(const int16_t *data, size_t n_adc, int16_t baseline);

	private:
};

sbndaq::CAENV1730Stream::CAENV1730Stream(fhicl::ParameterSet const & pset)
    : EDAnalyzer(pset)  {
  sbndaq::InitializeMetricManager(pset);
}

sbndaq::CAENV1730Stream::~CAENV1730Stream() {}

void sbndaq::CAENV1730Stream::analyze(art::Event const & evt)
{
  art::Handle< std::vector<artdaq::Fragment> > rawFragHandle; // it is a pointer to a vector of art fragments
  evt.getByLabel("daq","CAENV1730", rawFragHandle); // it says how many fragments are in an event

  if (rawFragHandle.isValid()) {
    for (size_t idx = 0; idx < rawFragHandle->size(); ++idx) { 
      const auto& frag((*rawFragHandle)[idx]); 
      CAENV1730Fragment bb(frag);

      CAENV1730FragmentMetadata const* metadata_ptr = bb.Metadata();
      CAENV1730Event const* event_ptr = bb.Event();

      CAENV1730EventHeader header = event_ptr->Header;


      //get the number of 32-bit words from the header
      uint32_t ev_size = header.eventSize;
      
      //use that to get the number of 16-bit words for each channel
      uint32_t ch_size = 2*(ev_size - sizeof(CAENV1730EventHeader)/sizeof(uint32_t))/16; 

      uint32_t n_channels = metadata_ptr->nChannels;

      std::cout << "Event Size: " << ev_size << std::endl;
      std::cout << "Channel Size: " << ch_size << std::endl;
      std::cout << "n channels: " << n_channels << std::endl;
      std::cout << "header size: " << sizeof(CAENV1730EventHeader) << std::endl;

      const int16_t* data = reinterpret_cast<const int16_t*>(frag.dataBeginBytes() + sizeof(CAENV1730EventHeader));
      (void) data;
      // int16_t baseline = Baseline(data, ch_size); 
      // double rms = RMS(data, ch_size, baseline);

      // send the metric
      sbndaq::sendMetric("n_channels", (long unsigned int) n_channels, 3, artdaq::MetricMode::Average);

    }
  }
}

int16_t sbndaq::CAENV1730Stream::Baseline(const int16_t *data, size_t n_adc) {
  return Mode(data, n_adc);
}

double sbndaq::CAENV1730Stream::RMS(const int16_t *data, size_t n_adc, int16_t baseline) {
  double ret = 0;
  for (size_t i = 0; i < n_adc; i++) {
    ret += (data[i] - baseline) * (data[i] - baseline);
  }
  return sqrt(ret / n_adc);
}

DEFINE_ART_MODULE(sbndaq::CAENV1730Stream)
//this is where the name is specified
