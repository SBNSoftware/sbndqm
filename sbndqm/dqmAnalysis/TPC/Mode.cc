#include <vector>
#include <array>

#include "Mode.hh"

int16_t Mode(const std::vector<int16_t> &adcs, unsigned n_skip_samples) {
  return Mode(&adcs[0], adcs.size(), n_skip_samples);
}

// Calculate the mode to find a baseline of the passed in waveform.
// Mode finding algorithm from: http://erikdemaine.org/papers/NetworkStats_ESA2002/paper.pdf (Algorithm FREQUENT)
int16_t Mode(const int16_t *adcs, size_t n_adcs, unsigned n_skip_samples) {
  // 10 counters seem good
  std::array<unsigned, 10> counters {}; // zero-initialize
  std::array<int16_t, 10> modes {};

  for (size_t adc_ind = 0; adc_ind < n_adcs; adc_ind += n_skip_samples) {
    int16_t val = adcs[adc_ind];

    int home = -1;
    // look for a home for the val
    for (size_t i = 0; i < modes.size(); i ++) {
      if (modes[i] == val) {
        home = (int)i; 
        break;
      }
    }
    // invade a home if you don't have one
    if (home < 0) {
      for (int i = 0; i < (int)modes.size(); i++) {
        if (counters[i] == 0) {
          home = i;
          modes[i] = val;
          break;
        }
      }
    }
    // incl if home
    if (home >= 0) counters[home] ++;
    // decl if no home
    else {
      for (int i = 0; i < (int)counters.size(); i++) {
        counters[i] = (counters[i]==0) ? 0 : counters[i] - 1;
      }
    }
  }
  // highest counter has the mode
  unsigned max_counters = 0;
  short ret = 0;
  for (int i = 0; i < (int)counters.size(); i++) {
    if (counters[i] > max_counters) {
      max_counters = counters[i];
      ret = modes[i];
    }
  }
  return ret;
}

