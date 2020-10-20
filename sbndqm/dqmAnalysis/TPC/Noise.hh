#ifndef _sbnddaq_analysis_Noise
#define _sbnddaq_analysis_Noise
#include <vector>
#include <array>

#include "PeakFinder.hh"

// keeps track of which regions of a waveform are suitable for noise calculations (i.e. don't contain signal)
namespace tpcAnalysis {
class NoiseSample {
public:
  // construct sample from peaks (signals)
  NoiseSample(std::vector<PeakFinder::Peak>& peaks, int16_t baseline, unsigned wvfm_size);
  // construct from a list of ranges that don't have signal
  NoiseSample(std::vector<std::array<unsigned, 2>> ranges, int16_t baseline): _ranges(ranges), _baseline(baseline) {}
  // zero initialize
  NoiseSample(): _baseline(0) {}

  // calculate the intersect of ranges with another sample
  NoiseSample Intersection(NoiseSample &other) { return DoIntersection(*this, other, _baseline); }

  float RMS(const std::vector<int16_t> &wvfm_self, unsigned max_sample=UINT_MAX) { return CalcRMS(wvfm_self, _ranges, _baseline); } 

  // Functions for quantifying coherent noise:
  float Covariance(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample=UINT_MAX);
  float Correlation(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample=UINT_MAX);
  // the "Sum RMS" of a sample with another sample
  float SumRMS(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample=UINT_MAX);
  // the "Sum RMS" of n samples
  static float ScaledSumRMS(std::vector<NoiseSample *>& other, std::vector<const std::vector<int16_t> *>& wvfm_other, unsigned max_sample=UINT_MAX);
  // "DNoise" with another sample
  float DNoise(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample=UINT_MAX);

  // re-calculate the baseline as taking the mean of all values in the noise ranges
  void ResetBaseline(const std::vector<int16_t> &wvfm_self);

  // get access to the ranges
  std::vector<std::array<unsigned, 2>> *Ranges() { return &_ranges; }
  // getter for the baseline
  int16_t Baseline() { return _baseline; }
private:
  static float CalcRMS(const std::vector<int16_t> &wvfm_self, std::vector<std::array<unsigned,2>> &ranges, int16_t baseline, unsigned max_sample=UINT_MAX);
  static NoiseSample DoIntersection(NoiseSample &me, NoiseSample &other, int16_t baseline=0.);

  std::vector<std::array<unsigned, 2>> _ranges;
  int16_t _baseline;
};

// helper function to sum a group of waveforms looking for e.g. coherent noise
void SumWaveforms(std::vector<int> &output, std::vector<const std::vector<int16_t>*>& waveforms, std::vector<int16_t>& baselines);

} // namespace tpcAnalysis
#endif
