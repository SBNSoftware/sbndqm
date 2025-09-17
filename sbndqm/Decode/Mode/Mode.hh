#ifndef DAQANALYSIS_MODE_HH
#define DAQANALYSIS_MODE_HH

#include <cstdint>
#include <vector>
#include <array>

#include "Mode.hh"

// Calculate the mode to find a baseline of the passed in waveform.
// Mode finding algorithm from: http://erikdemaine.org/papers/NetworkStats_ESA2002/paper.pdf (Algorithm FREQUENT)

// Citation:
// Erik D. Demaine, Alejandro L ́opez-Ortiz, and J. Ian Munro. Frequency
// estimation of internet packet streams with limited space. In Rolf
// M ̈ohring and Rajeev Raman, editors, Algorithms — ESA 2002, pages
// 348–360, Berlin, Heidelberg, 2002. Springer Berlin Heidelberg.

using std::size_t;

int16_t Mode(const int16_t *data, size_t n_data, unsigned n_skip_samples=1);

int16_t Mode(const std::vector<int16_t> &adcs, unsigned n_skip_samples=1);

#endif
