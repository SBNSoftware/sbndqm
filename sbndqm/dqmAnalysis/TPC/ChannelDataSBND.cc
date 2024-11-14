#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream> 
#include <stdlib.h>

#include "PeakFinder.hh"
#include "ChannelDataSBND.hh"

float tpcAnalysis::ChannelDataSBND::meanPeakHeight() {
  if (peaks.size() == 0) {
    return 0;
  } 

  int total = 0;
  for (unsigned i = 0; i < peaks.size(); i++) {
    total += peaks[i].amplitude;
  }

  return ((float) total) / peaks.size();
}

float tpcAnalysis::ChannelDataSBND::meanPeakHeight(const std::vector<art::Ptr<recob::Hit> > &hits) {
  if(hits.size() == 0) {
    return 0;
  }
  int total = 0;
  for(std::vector<art::Ptr<recob::Hit> >::const_iterator hit_iter=hits.begin();hit_iter!=hits.end(); ++hit_iter){
    total += (*hit_iter)->PeakAmplitude();
  }

  return ((float) total)/ hits.size();
}


// only count up peaks
float tpcAnalysis::ChannelDataSBND::Occupancy() {
  float n_peaks = 0;
  for (unsigned i = 0; i < peaks.size(); i++) {
    if (peaks[i].is_up) {
      n_peaks += 1;
    }
  }
  return n_peaks;
}

std::string tpcAnalysis::ChannelDataSBND::Print() {
  std::stringstream buffer;
  buffer << "baseline: " << baseline << std::endl;
  buffer << "rms: " << rms << std::endl;
  buffer << "timestamp: " << timestamp << std::endl;
  buffer << "channel_no: " << channel_no << std::endl;
  buffer << "empty: " << empty << std::endl;
  buffer << "threshold: " << threshold << std::endl; 
  buffer << "next_channel_dnoise: " << next_channel_dnoise << std::endl;

  buffer << "peaks: [" << std::endl;
  for (auto &peak: peaks) {
    buffer << "  {" << std::endl;
    buffer << peak.Print() << std::endl;
    buffer << "  }" << std::endl;
  }
  buffer << "]" << std::endl;

  buffer << "noise_ranges: [" << std::endl;
  for (auto &range: noise_ranges) {
    buffer << "  [ " <<  range[0] << ", " << range[1] << "],"<< std::endl;
  }
  buffer << "]" << std::endl;

  return buffer.str();
}

