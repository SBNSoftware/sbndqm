#ifndef _sbnddaq_analysis_PeakFinder
#define _sbnddaq_analysis_PeakFinder
#include <vector>
#include <array>
#include <string>
#include <float.h>

#include "canvas/Persistency/Common/Ptr.h"
#include "lardataobj/RecoBase/Hit.h"

namespace tpcAnalysis {
// Reinventing the wheel: search for a bunch of peaks in a set of data
// 
// Implementation: searches for points above some threshold (requiring a 
// good basline) and tries to make peaks for them.
class PeakFinder {
public:
  class Peak {
  public:
    uint16_t amplitude;
    unsigned peak_index;
    unsigned start_tight;
    unsigned start_loose;
    unsigned end_tight;
    unsigned end_loose;
    bool is_up;

    Peak() {
      reset();
    }
    void reset() {
      amplitude = 0;
      peak_index = 0;
      start_tight = 0;
      start_loose = 0;
      end_tight = 0;
      end_loose = 0;
    }

    std::string Print() const;

  };

  enum plane_type { unspecified, induction, collection };

  // initialize internal vector of peaks from art product
  explicit PeakFinder(const std::vector<art::Ptr<recob::Hit> > &hits);

  // generate list of peaks by providing waveform -- does hitfinding internally
  PeakFinder(std::vector<int16_t> &waveform, int16_t baseline, float threshold, 
      unsigned n_smoothing_samples=1, unsigned n_above_threshold=0, plane_type plane=unspecified);
  inline std::vector<Peak> *Peaks() { return &_peaks; }
private:
  Peak FinishPeak(Peak peak, std::vector<int16_t> *waveform, unsigned n_smoothing_samples, int16_t baseline, bool up_peak, unsigned index);
  void matchPeaks(unsigned match_range);
  std::vector<int16_t> _smoothed_waveform;
  std::vector<Peak> _peaks;
};

// Classes for different types of threshold calculation

// gets threshold from gaussian fit to histogram of ADC values
class Threshold {
public:
  Threshold(std::vector<int16_t> &waveform, int16_t baseline, float n_sigma=5., bool verbose=true);

  inline float Val() { return _threshold; }
private:
  float _threshold;
};

// gets threshold from running average of rms values
class RunningThreshold {
public:
  RunningThreshold(): _rms_ind(0), _n_past_rms(0) { std::fill(_past_rms.begin(), _past_rms.end(), 0); }

  float Threshold(std::vector<int16_t> &waveform, int16_t baseline, float n_sigma=5.);
  void AddRMS(float rms);

private:
  std::array<float, 10> _past_rms;
  unsigned _rms_ind;
  unsigned _n_past_rms;
};

} // end namespace tpcAnalysis

#endif
