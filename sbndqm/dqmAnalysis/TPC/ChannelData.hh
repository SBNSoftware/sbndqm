#ifndef _sbnddaq_analysis_WaveformData
#define _sbnddaq_analysis_WaveformData

#include <vector>
#include <array>
#include <string>

#include "PeakFinder.hh"
#include "canvas/Persistency/Common/Ptr.h"
#include "lardataobj/RecoBase/Hit.h"

namespace tpcAnalysis {
class ChannelData {
public:
  unsigned channel_no;
  bool empty;
  int16_t baseline;
  int16_t max;
  int16_t min;
  float rms;
  float next_channel_dnoise;
  float threshold;
  std::vector<int16_t> waveform;
  std::vector<double> fft_real;
  std::vector<double> fft_imag;
  std::vector<PeakFinder::Peak> peaks;
  std::vector<std::array<unsigned, 2>> noise_ranges;

  std::string Print();

  float mean_peak_height;
  float occupancy;

  float meanPeakHeight();
  float meanPeakHeight(const std::vector<art::Ptr<recob::Hit> > &hits);
  float Occupancy();

  // zero initialize
  explicit ChannelData(unsigned channel=0):
    channel_no(channel),
    empty(true /* except for empty by default*/),
    baseline(0),
    max(0),
    min(0),
    rms(0),
    next_channel_dnoise(0),
    threshold(0),
    mean_peak_height(0),
    occupancy(0)
  {}
};

class ReducedChannelData {
public:
  unsigned channel_no;
  bool empty;
  int16_t baseline;
  float rms;
  float occupancy;
  float mean_peak_amplitude;

  // zero initialize
  explicit ReducedChannelData(unsigned channel=0):
    channel_no(channel),
    empty(true /* except for empty by default*/),
    baseline(0),
    rms(0),
    occupancy(0),
    mean_peak_amplitude(0)
  {}

  ReducedChannelData(ChannelData &channel_data) {
    channel_no = channel_data.channel_no;
    empty = channel_data.empty;
    baseline = channel_data.baseline;
    rms = channel_data.rms;
    occupancy = channel_data.occupancy;
    mean_peak_amplitude = channel_data.mean_peak_height;
  }
};

}

#endif
