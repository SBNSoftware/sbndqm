#include <vector>
#include <array>
#include <numeric>
#include <math.h> 
#include <stdlib.h>
#include <iostream>

#include "Noise.hh"

using namespace tpcAnalysis;

NoiseSample::NoiseSample(std::vector<PeakFinder::Peak>& peaks, int16_t baseline, unsigned wvfm_size) {
  // we assume here that the vector of peaks are "sorted"
  // that peak[i].start_loose <= peak[i+1].start_loose and
  // that peak[i].end_loose <= peak[i+1].end_loose
  unsigned min = 0;
  unsigned peak_ind = 0;
  // noise samples are where the peaks aren't
  while (peak_ind < peaks.size()) {
    if (min < peaks[peak_ind].start_loose) {
      _ranges.emplace_back( std::array<unsigned,2>{min, peaks[peak_ind].start_loose-1});
    }
    min = peaks[peak_ind].end_loose+1;
    peak_ind ++;
  }
  if (min < wvfm_size-1) {
    _ranges.emplace_back( std::array<unsigned,2>{min, wvfm_size - 1} );
  } 
  _baseline = baseline;
}

float NoiseSample::CalcRMS(const std::vector<int16_t> &wvfm_self, std::vector<std::array<unsigned,2>> &ranges, int16_t baseline, unsigned max_sample) {
  unsigned n_samples = 0;
  double ret = 0;
  // iterate over the regions w/out signal
  for (auto &range: ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      ret += (wvfm_self[i] - baseline) * (wvfm_self[i] - baseline);

      if (n_samples == max_sample) {
        return sqrt((double)ret / n_samples);
      }
    }
  }

  return sqrt((double)ret / n_samples);
}

NoiseSample NoiseSample::DoIntersection(NoiseSample &me, NoiseSample &other, int16_t baseline) {
  std::vector<std::array<unsigned, 2>> ranges;
  unsigned self_ind = 0;
  unsigned other_ind = 0;
  while (self_ind < me._ranges.size() && other_ind < other._ranges.size()) {
    // determine if there is a valid intersection
    bool is_intersection = (me._ranges[self_ind][1] >= other._ranges[other_ind][0]) &&
                           (me._ranges[self_ind][0] <= other._ranges[other_ind][1]);
    if (is_intersection) {
      unsigned intersection_lo = std::max(me._ranges[self_ind][0], other._ranges[other_ind][0]);
      unsigned intersection_hi = std::min(me._ranges[self_ind][1], other._ranges[other_ind][1]);
      ranges.emplace_back( std::array<unsigned,2>{intersection_lo, intersection_hi} );
    }

    // determine which ind to incl
    if (me._ranges[self_ind][1] < other._ranges[other_ind][1]) self_ind ++;
    else other_ind ++;

  }

  return NoiseSample(ranges, baseline);
}

float NoiseSample::Covariance(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample) {
  NoiseSample joint = Intersection(other);
  unsigned n_samples = 0;
  int ret = 0;
  // iterate over the regions w/out signal
  for (auto &range: joint._ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      ret += (wvfm_self[i] - _baseline) * (wvfm_other[i] - other._baseline);
      if (n_samples == max_sample) {
        return ((float)ret) / n_samples;
      }
    }
  }
  return ((float)ret) / n_samples;
}

float NoiseSample::Correlation(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample) {
  NoiseSample joint = Intersection(other);
  float scaling = CalcRMS(wvfm_self, joint._ranges, _baseline, max_sample) * CalcRMS(wvfm_other, joint._ranges, other._baseline, max_sample);
  return Covariance(wvfm_self, other, wvfm_other, max_sample) / scaling;
}

float NoiseSample::SumRMS(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample) {
  NoiseSample joint = Intersection(other);
  unsigned n_samples = 0;
  int ret = 0;
  // iterate over the regions w/out signal
  for (auto &range: joint._ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      int16_t val = wvfm_self[i] - _baseline + wvfm_other[i] - other._baseline;
      ret += val * val;
      if (n_samples == max_sample) {
        return sqrt(((float)ret) / n_samples);
      }
    }
  }
  return sqrt(((float)ret) / n_samples);
}

float NoiseSample::ScaledSumRMS(std::vector<NoiseSample *>& noises, std::vector<const std::vector<int16_t> *>& waveforms, unsigned max_sample) {
  // calculate the joint noise sample over all n samples
  // n must be >= 2
  NoiseSample joint = DoIntersection(*noises[0], *noises[1]);
  for (unsigned i = 2; i < noises.size(); i++) {
    joint = DoIntersection(joint, *noises[i]);
  }

  unsigned n_samples = 0;
  int ret = 0;
  // iterate over the regions w/out signal
  for (auto &range: joint._ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      n_samples ++;
      int sample = 0;
      for (unsigned wvfm_ind = 0; wvfm_ind < noises.size(); wvfm_ind++) {
        sample += (*waveforms[wvfm_ind])[i] - noises[wvfm_ind]->_baseline;
      }
      ret += sample * sample;
      if (n_samples == max_sample) break;
    }
    if (n_samples == max_sample) break;
  }
  float sum_rms = ((float)ret) / n_samples; 

  float rms_all = 0;
  for (unsigned wvfm_ind = 0; wvfm_ind < noises.size(); wvfm_ind++) {
    rms_all += CalcRMS(*waveforms[wvfm_ind], joint._ranges, noises[wvfm_ind]->_baseline, max_sample);
  }
  // send uncorrelated sum-rms value to 0
  float scale_sub = rms_all * sqrt((float) noises.size());
  // and send fully correlated sum-rms value to 1
  float scale_div = rms_all * noises.size() - scale_sub;

  return (sum_rms - scale_sub) / scale_div; 
}

float NoiseSample::DNoise(const std::vector<int16_t> &wvfm_self, NoiseSample &other, const std::vector<int16_t> &wvfm_other, unsigned max_sample) {
  NoiseSample joint = Intersection(other);

  unsigned n_samples = 0;
  int noise = 0;
  // iterate over the regions w/out signal
  for (auto &range: joint._ranges) {
    for (unsigned i = range[0]; i <= range[1]; i ++) {
      n_samples ++;
      int16_t val = (wvfm_self[i] - _baseline) - (wvfm_other[i] - other._baseline);
      noise += val * val;
      if (n_samples == max_sample) {
        return sqrt(((float) noise) / n_samples);
      }
    }
  }
  return sqrt(((float) noise) / n_samples);
}

// calculated the mean of all adc values in noise ranges, and sets that as baseline
void NoiseSample::ResetBaseline(const std::vector<int16_t> &wvfm_self) {
  int total = 0;
  int n_values = 0;
  for (auto &range: _ranges) {
    for (unsigned i = range[0]; i <= range[1]; i++) {
      total += wvfm_self[i];
      n_values ++;
    }
  }
  // Don't crash if there are no values in the noise range, even though this would 
  // obviously be a very bad thing. It's more important to persit to maintain analysis.
  if (n_values == 0) return;

  _baseline = total / n_values;
}

// sum a group of waveforms looking for e.g. coherent noise
// assumes output is of size output_size
void SumWaveforms(std::vector<int> &output, std::vector<const std::vector<int16_t>*>& waveforms, std::vector<int16_t> &baselines) {
  size_t output_size = waveforms[0]->size();

  for (size_t adc_ind = 0; adc_ind < output_size; adc_ind++) {
    // make space
    output.push_back(0);
    // sum
    for (size_t waveform_ind = 0; waveform_ind < waveforms.size(); waveform_ind++) {
      output[adc_ind] += (*waveforms[waveform_ind])[adc_ind] - baselines[waveform_ind];
    } 
  }
}



