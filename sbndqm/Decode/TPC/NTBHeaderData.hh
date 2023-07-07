#ifndef _sbnddaq_analysis_NTBHeaderData
#define _sbnddaq_analysis_NTBHeaderData

#include <string>
#include <iostream>
#include <sstream> 
#include <stdlib.h>
#include <ctime>

namespace ntbAnalysis {

  class NTBHeaderData {
  public:

    uint16_t sample_ntb; // 2MHz sample number from NTB data
    uint32_t frame_ntb;
    uint32_t event_ntb;

    //Default constructor with member variables initialized to specific values
    NTBHeaderData():
      sample_ntb(0xFFFF),
      frame_ntb(0xBEEFDEAD),
      event_ntb(0xBEEFDEAD)

    {}
    // print the data -- for debugging
    std::string Print() const {
      std::stringstream buffer;
      buffer << "sample: " << sample_ntb << std::endl;
      buffer <<"frame: " << frame_ntb << std::endl;
      buffer <<"event: " << event_ntb << std::endl;
      return buffer.str();
    }
  };
}

#endif
