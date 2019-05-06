#include <vector>
#include <cassert>
#include <iostream>

#include "fftw3.h"

#include "FFT.hh"

FFTManager::FFTManager(unsigned input_size) {
  _is_allocated = false;
  if (input_size != 0) { 
    Set(input_size);
  }
}

void FFTManager::Set(unsigned input_size) {
  _input_size = input_size;
  // output size of a 1d real FFT
  _output_size = input_size/2 + 1;
  Alloc();
}

void FFTManager::Alloc() {
  if (_is_allocated) {
    DeAlloc();
  }
  //unsigned flags = FFTW_ESTIMATE;
  unsigned flags = FFTW_MEASURE;
  _input_array = fftw_alloc_real(_input_size);
  _output_array = fftw_alloc_complex(_output_size);
  _plan = fftw_plan_dft_r2c_1d(_input_size, _input_array, _output_array, flags);
  _is_allocated = true;
}

void FFTManager::DeAlloc() {
  if (_is_allocated) {
    fftw_free(_input_array);
    fftw_free(_output_array);
    fftw_destroy_plan(_plan);
  }
  _is_allocated = false;
}

void FFTManager::Execute() {
  fftw_execute(_plan);
}

// get pointer to ith input
double *FFTManager::InputAt(const int index) {
  assert(_is_allocated);
  assert(index >= 0);
  assert((unsigned int)index < _input_size);
  return &_input_array[index];
}

double FFTManager::ReOutputAt(const int index) {
  assert(_is_allocated);
  assert(index >= 0);
  assert((unsigned int)index < _output_size);
  return _output_array[index][0];
}

double FFTManager::ImOutputAt(const int index) {
  assert(_is_allocated);
  assert(index >= 0);
  assert((unsigned int)index < _output_size);
  return _output_array[index][1];
}

double FFTManager::AbsOutputAt(const int index) {
  assert(_is_allocated);
  assert(index >= 0);
  assert((unsigned int)index < _output_size);
  return _output_array[index][0]*_output_array[index][0] + _output_array[index][1]*_output_array[index][1];
}

FFTManager::~FFTManager() {
  DeAlloc();
}



