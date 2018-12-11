#ifndef _sbnddaq_analysis_FFT
#define _sbnddaq_analysis_FFT
#include <vector>

#include "fftw3.h"

// Computes the Discrete Fourier Transform of the _real_ input data.
// Output has a size 2 *(n/2 + 1) where n is the size of the input data.
// Returned array must be free'd with fftw_free
class FFTManager {
public:
  // Make a new FFT manager and allocate a setup for an input array of size input_size
  explicit FFTManager(unsigned input_size);
  // Make a new FFT manager and don't allocate
  FFTManager() {}
  // allocate a setup for an input array of size input_size (NOTE: is idempotent)
  void Set(unsigned input_size);
  // execute the FFT
  void Execute();
  // get a member of the input array
  double *InputAt(const int index);
  // get a member of the output array
  double ReOutputAt(const int index);
  double ImOutputAt(const int index);
  double AbsOutputAt(const int index);
  
  // input/output sizes
  unsigned InputSize() {return _input_size;}
  unsigned OutputSize() {return _output_size;}

  ~FFTManager();

protected:
  // Internal functions
  void Alloc();
  void DeAlloc();

  unsigned _input_size;
  unsigned _output_size;
  bool _is_allocated;
  fftw_complex *_output_array;
  double *_input_array;
  fftw_plan _plan;
};

#endif
