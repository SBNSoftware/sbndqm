#include <vector>
#include <numeric>
#include <cassert>
#include <float.h>
#include <algorithm>
#include <string>
#include <iostream>
#include <sstream> 
#include <cmath>

#include "TH1D.h"
#include "TF1.h"

#include "PeakFinder.hh"
#include "Noise.hh"
#include "Analysis.hh"

using namespace tpcAnalysis;

inline bool fitDownPeak(PeakFinder::plane_type plane) {
  return plane == PeakFinder::induction || plane == PeakFinder::unspecified;
}

inline bool doMatchPeaks(PeakFinder::plane_type plane) {
  return plane == PeakFinder::induction;
}

PeakFinder::PeakFinder(const std::vector<art::Ptr<recob::Hit> > &hits) {
  //Creates peak objects used for the gettting the channel info by using the hits found using RawHitFinder.                                
  for(std::vector<art::Ptr<recob::Hit> >::const_iterator hit_iter=hits.begin(); hit_iter!=hits.end(); ++hit_iter){
    PeakFinder::Peak peak;
    peak.amplitude = (*hit_iter)->PeakAmplitude();
    peak.peak_index = (*hit_iter)->PeakTime();
    peak.start_tight = (*hit_iter)->StartTick();
    peak.start_loose = (*hit_iter)->StartTick();
    peak.end_tight = (*hit_iter)->EndTick();
    peak.end_loose = (*hit_iter)->EndTick();
    // The RawHitFinder definition of a "hit" is slightly different from that of PeakFinder "peak"
    // Peakfinder counds an induction signal as two peaks, one up and one down. RawHitFidner counts
    // it as one hit. To make the semantics the same, make every RawHitFinder peak an up_peak
    // so that the occupancy and mean peak heights are calculated correctly
    peak.is_up = true;
    _peaks.emplace_back(peak);
  }

}

PeakFinder::PeakFinder(std::vector<int16_t> &inp_waveform, int16_t baseline, float threshold, unsigned n_smoothing_samples, unsigned n_above_threshold, plane_type plane ) {
  // number of smoothing samples must be odd to make sense
  assert(n_smoothing_samples % 2 == 1);

  // smooth out input waveform if need be
  if (n_smoothing_samples > 1) {
    unsigned smoothing_per_direction = n_smoothing_samples / 2;
    for (unsigned i = smoothing_per_direction; i < inp_waveform.size() - smoothing_per_direction; i++) {
      unsigned begin = i - smoothing_per_direction;
      unsigned end = i + smoothing_per_direction + 1;
      int16_t average = std::accumulate(inp_waveform.begin() + begin, inp_waveform.begin() + end, 0.0) / n_smoothing_samples;
      
      _smoothed_waveform.push_back(average); 
    } 
  }

  // use the smoothed waveform or the passed in waveform
  std::vector<int16_t> *waveform = (n_smoothing_samples > 1) ? &_smoothed_waveform : &inp_waveform;

  // iterate through smoothed samples
  bool inside_peak = false;
  // whether peak points up or down
  bool up_peak = false;
  PeakFinder::Peak peak;
  // keep track of how many points above threshold
  unsigned n_points = 0;

  for (unsigned i = 0; i < waveform->size(); i++) {
    int16_t dat = (*waveform)[i];
    // detect a new peak, or continue on the current one

    // up-peak
    // only take this case if looking for new peak or already in up-peak
    if (dat > baseline + threshold && (!inside_peak || up_peak)) {
      if (!inside_peak) {
        // increment above threshold
        n_points ++;
        // it's a new peak!
        if (n_points == n_above_threshold) {
          // reset points above threshold counting
          n_points = 0;
          // setup peak
          peak.amplitude = 0;
	  peak.start_tight = i;
	  inside_peak = true;
	  up_peak = true;
	  peak.is_up = true;
        }
      }

      if (inside_peak) {
        // always use original waveform to determine amplitude
        // define amplitude as distance from baseline
        uint16_t amplitude = abs(inp_waveform[i] - baseline);
        if (amplitude > peak.amplitude) {
          peak.amplitude = amplitude;
          peak.peak_index = i;
        }
      }
    }
    // down-peak
    // only take this case if looking for new peak or already in down-peak
    else if (fitDownPeak(plane) && dat < baseline - threshold && (!inside_peak || !up_peak)) {
      if (!inside_peak) {
        // increment above threshold
        n_points ++;
        // it's a new peak!
        if (n_points == n_above_threshold) {
          // reset points above threshold counting
          n_points = 0;
          // setup peak
	  peak.amplitude = 0;
	  peak.start_tight = i;
	  inside_peak = true;
	  up_peak = false;
	  peak.is_up = false;
        }
      }
      if (inside_peak) {
        // always use original waveform to determine amplitude
        // define amplitude as distance from baseline
        uint16_t amplitude = abs(inp_waveform[i] - baseline);
        if (amplitude > peak.amplitude) {
          peak.amplitude = amplitude;
          peak.peak_index = i;
        }
      }
    }
    // this means we're at the end of a peak. 
    // process it and set up the next one
    else if (inside_peak) {
      inside_peak = false;
      peak = FinishPeak(peak, waveform, n_smoothing_samples, baseline, up_peak, i);
      _peaks.emplace_back(peak);
      peak = PeakFinder::Peak();
    }
    else { 
      // not inside peak, not at end of peak
      // reset points above threshold counter
      n_points = 0;
    }
  }
  // finish peak if we're inside one at the end
  if (inside_peak) {
    peak = FinishPeak(peak, waveform, n_smoothing_samples, baseline, up_peak, waveform->size()-1);
    _peaks.emplace_back(peak);
  }

  // match peaks
  if (doMatchPeaks(plane)) {
    // set matching distance based on smoothing samples (for now)
    matchPeaks(n_smoothing_samples*2);
  }
}


PeakFinder::Peak PeakFinder::FinishPeak(PeakFinder::Peak peak, std::vector<int16_t> *waveform, unsigned n_smoothing_samples, int16_t baseline, bool up_peak, unsigned index) {
  peak.end_tight = index;
  // find the upper and lower bounds to determine the max width
  peak.start_loose = peak.start_tight;
  unsigned n_at_baseline = 0;
  while (peak.start_loose > 0) {
    if ((up_peak && (*waveform)[peak.start_loose] <= baseline) ||
       (!up_peak && (*waveform)[peak.start_loose] >= baseline)) {

      n_at_baseline ++;
    }
    // define the lower bound to be where the point where the waveform
    // has gone under the baseline twice
    if (n_at_baseline >= 2) {
      break;
    }
    peak.start_loose --;
  }
  // set start_loose such that it isn't under the influence of any points inside peak
  peak.start_loose = (peak.start_loose >= n_smoothing_samples/2) ? (peak.start_loose - n_smoothing_samples/2) : 0;
  
  // now find upper bound on end
  peak.end_loose = peak.end_tight;
  n_at_baseline = 0;
  while (peak.end_loose < waveform->size()-1) {
    if ((up_peak && (*waveform)[peak.end_loose] <= baseline) ||
       (!up_peak && (*waveform)[peak.end_loose] >= baseline)) {
      n_at_baseline ++;
    }
    if (n_at_baseline > 2) {
      break;
    }
    peak.end_loose ++;
  }
  // set end_loose such that it isn't under the influence of any points inside peak
  peak.end_loose = std::min(peak.end_loose + n_smoothing_samples/2, (unsigned)waveform->size()-1);
  return peak;
}

// match up peak - down peak pairs for induction planes
void PeakFinder::matchPeaks(unsigned match_range) {
  std::vector<PeakFinder::Peak> pruned_peaks;

  bool last_was_up_peak = false;
  for (unsigned i = 0; i < _peaks.size(); i++) {
    // look for down peak that immediately follows up peak
    if (!_peaks[i].is_up && last_was_up_peak) {
      // make sure start of down peak is within range of end of up peak
      if ((i > 0) &&
          ((_peaks[i].start_loose < _peaks[i-1].end_loose) ||
           (_peaks[i].start_loose - _peaks[i-1].end_loose < match_range))) {
        pruned_peaks.push_back(_peaks[i-1]);
        pruned_peaks.push_back(_peaks[i]);
      }
    }
    last_was_up_peak = _peaks[i].is_up;
  }
  // set the peaks to the pruned version
  _peaks = pruned_peaks;
}

// Print for debugging purposes
std::string PeakFinder::Peak::Print() const {
  std::stringstream buffer;
  buffer << "    amplitude: " << amplitude << std::endl;
  buffer << "    peak_index: " << peak_index << std::endl;
  buffer << "    start_tight: " << start_tight << std::endl;
  buffer << "    start_loose: " << start_loose << std::endl;
  buffer << "    end_loose: " << end_loose << std::endl;
  buffer << "    end_tight: " << end_tight << std::endl;
  buffer << "    is_up: " << is_up << std::endl;

  return buffer.str();

}


// Calculate the threshold by fitting a gaussian to a histogram of ADC values from the waveform
Threshold::Threshold(std::vector<int16_t> &waveform, int16_t baseline, float n_sigma, bool verbose) {
  int16_t min = *std::min_element(waveform.begin(), waveform.end());
  int16_t max = *std::max_element(waveform.begin(), waveform.end());
  size_t length = waveform.size();

  // histogram of adc values
  TH1D hist("waveform", "waveform", length/100, min, max);
  for (int16_t dat: waveform) {
    hist.Fill(dat);
  }
  if (verbose) {
    std::cout << "MEAN: " << hist.GetMean() << std::endl;
    std::cout << "RMS: " << hist.GetRMS() << std::endl;
  }

  TF1 fit("fit", "gaus");
  fit.SetParameter(0, baseline);
  fit.SetParameter(1, hist.GetMean());
  fit.SetParameter(2, hist.GetRMS());

  // only hit over a logical region
  fit.SetRange( hist.GetMean() - n_sigma*hist.GetRMS(), hist.GetMean() + n_sigma*hist.GetRMS() );

  if (verbose) {
    // if verbose, print out info associated w/fit
    hist.Fit(&fit, "R");
  }
  else {
    hist.Fit(&fit, "RQ");
  }

  // set thresholds at n_sigma
  _threshold = baseline + n_sigma*fit.GetParameter(2);
}

// gets the RMS from a wavefrom including any present signal 
// i.e. will always overestimate the "true" RMS unless no signal is present
float rawRMS(std::vector<int16_t> &waveform, int16_t baseline) {
  NoiseSample temp({{0, (unsigned)waveform.size() -1}}, baseline);
  return temp.RMS(waveform);
}

// get the threshold from a running average of rms values
float RunningThreshold::Threshold(std::vector<int16_t> &waveform, int16_t baseline, float n_sigma) {
  // if there's no history, just use the raw RMS
  if (_n_past_rms == 0) {
    // 2x penalty since rawRMS will overestimate the true RMS
    // edit: no penalty for now
    return rawRMS(waveform, baseline) * n_sigma;
  }
  else {
    float rms = 0;
    for (unsigned i = 0; i < _n_past_rms; i++) {
      rms += _past_rms[i];
    }
    rms = rms / _n_past_rms;
    return rms * n_sigma;
  }
}

// add to running average
void RunningThreshold::AddRMS(float rms) {
  // don't save nan values
  if (std::isnan(rms)) return;

  if (_n_past_rms < 10) _n_past_rms ++;
  _past_rms[_rms_ind] = rms;
  _rms_ind = (_rms_ind + 1 ) % _past_rms.size();
}




