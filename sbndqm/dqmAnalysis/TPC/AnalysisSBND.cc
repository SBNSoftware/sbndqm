//some standard C++ includes
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <numeric>
#include <cmath>
#include <getopt.h>
#include <chrono>
#include <float.h>

//some ROOT includes
#include "TInterpreter.h"
#include "TROOT.h"
#include "TH1F.h"
#include "TTree.h"
#include "TFile.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TGraph.h"
#include "TFFTReal.h"

// art includes
#include "canvas/Utilities/InputTag.h"
//#include "canvas/Persistency/Common/FindMany.h"
//#include "canvas/Persistency/Common/FindOne.h"
#include "canvas/Persistency/Common/FindOneP.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "messagefacility/MessageLogger/MessageLogger.h"  
#include "art_root_io/TFileService.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RawData/raw.h"
#include "canvas/Persistency/Provenance/Timestamp.h"

#include "AnalysisSBND.hh"
#include "ChannelDataSBND.hh"
#include "sbndqm/Decode/TPC/HeaderData.hh"
#include "FFT.hh"
#include "Noise.hh"
#include "PeakFinder.hh"
#include "sbndqm/Decode/Mode/Mode.hh"
#include "sbndaq-artdaq-core/Overlays/SBND/NevisTPCFragment.hh"

using namespace tpcAnalysis;

AnalysisSBND::AnalysisSBND(fhicl::ParameterSet const & p) :
  _channel_info(p.get<fhicl::ParameterSet>("channel_info")), // get the channel info
  _config(p),
  _channel_index_map(_channel_info.NChannels()),
  _per_channel_data(_channel_info.NChannels()),
  _per_channel_data_reduced((_config.reduce_data) ? _channel_info.NChannels() : 0), // setup reduced event vector if we need it
  _noise_samples(_channel_info.NChannels()),
  _header_data(std::max(_config.n_headers,0)),
  _thresholds( (_config.threshold_calc == 3) ? _channel_info.NChannels() : 0),
  _fft_manager(  (_config.static_input_size > 0) ? _config.static_input_size: 0)

{
  _event_ind = 0;
}

AnalysisSBND::AnalysisSBNDConfig::AnalysisSBNDConfig(const fhicl::ParameterSet &param) {
  // set up config

  // whether to print stuff
  verbose = param.get<bool>("verbose", false);

  // configuring analysis code:

  // thresholds for peak finding
  // 0 == use static threshold (set by "threshold" fhicl parameter)
  // 1 == use gauss fitter rms
  // 2 == use raw rms
  // 3 == use rolling average of rms
  threshold_calc = param.get<unsigned>("threshold_calc", 0);
  threshold_sigma = param.get<float>("threshold_sigma", 5.);
  threshold = param.get<float>("threshold", 100);

  // determine method to get noise sample
  // 0 == use first `n_noise_samples`
  // 1 == use peakfinding
  noise_range_sampling = param.get<unsigned>("noise_range_sampling",0);

  // whether to use plane data in peakfinding
  use_planes = param.get<bool>("use_planes", false);

  // method to calculate baseline:
  // 0 == assume baseline is 0
  // 1 == assume baseline is in digits.GetPedestal()
  // 2 == use mode finding to get baseline
  baseline_calc = param.get<unsigned>("baseline_calc", 1);

  // whether to refine the baseline by calculating the mean
  // of all adc values
  refine_baseline = param.get<bool>("refine_baseline", false);

  // sets percentage of mode samples to be 100 / n_mode_skip
  n_mode_skip = param.get<unsigned>("n_mode_skip", 1);
 
  // only used if noise_range_sampling == 0
  // number of samples in noise sample
  n_noise_samples = param.get<unsigned>("n_noise_samples", 20);

  n_max_noise_samples = param.get<unsigned>("n_max_noise_samples", UINT_MAX);

  // number of samples to average in each direction for peak finding
  n_smoothing_samples = param.get<unsigned>("n_smoothing_samples", 1);
  // number of consecutive samples that must be above threshold to count as a peak
  n_above_threshold = param.get<unsigned>("n_above_threshold", 1);

  // Number of input adc counts per waveform. Set to negative if unknown.
  // Setting to some positive number will speed up FFT's.
  static_input_size = param.get<int>("static_input_size", -1);
  // how many headers to expect (set to negative if don't process) 
  // Expects the passed in HeaderData objects to have "index" values in [0, n_headers)
  n_headers = param.get<int>("n_headers", -1);

  // whether to calculate/save certain things
  fft_per_channel = param.get<bool>("fft_per_channel", false);
  fill_waveforms = param.get<bool>("fill_waveforms", false);
  reduce_data = param.get<bool>("reduce_data", false);
  timing = param.get<bool>("timing", false);

  find_signal = param.get<bool>("find_signal", true);

  // name of producer of raw::RawDigits
  //std::string producers = param.get<std::string>("producer_name");
  producers = param.get<std::vector<std::string>>("raw_digit_producers");
  header_producer = param.get<std::string>("header_producer");

  instance = param.get<std::string>("raw_digit_instance", "");  

}

void AnalysisSBND::AnalyzeEvent(art::Event const & event) {
  _raw_digits_handle.clear();
  _raw_timestamps_handle.clear();

  _event_ind ++;

   std::vector<long> ts_vec;
   ts_vec.clear();
   std::vector<long> ts_hist_vec;
   ts_hist_vec.clear();
   npeak = 0;

  // clear out containers from last iter
  for (unsigned i = 0; i < _channel_info.NChannels(); i++) {
    _per_channel_data[i].waveform.clear();
    _per_channel_data[i].fft_real.clear();
    _per_channel_data[i].fft_imag.clear();
    _per_channel_data[i].fft_mag.clear();
    _per_channel_data[i].peaks.clear();
    _per_channel_data[i] = ChannelDataSBND(i); // reset data
  }

  // get the raw digits
  for (const std::string &prod: _config.producers) {
    art::Handle<std::vector<raw::RawDigit>> digit_handle;
    if (_config.instance.size()) {
      event.getByLabel(prod, _config.instance, digit_handle);
    }
    else {
      event.getByLabel(prod, digit_handle);
    }

    // exit if the data isn't present
    if (!digit_handle.isValid()) {
      std::cerr << "Error: missing digits with producer (" << prod << ")" << std::endl;
      return;
    }
    art::fill_ptr_vector(_raw_digits_handle, digit_handle);
  }

  // get the timestamps
  for (const std::string &prod: _config.producers) {
    art::Handle<std::vector<raw::RDTimeStamp>> timestamp_handle;
    if (_config.instance.size()) {
      event.getByLabel(prod, _config.instance, timestamp_handle);
    }
    else {
      event.getByLabel(prod, timestamp_handle);
    }

    // exit if the data isn't present
    if (!timestamp_handle.isValid()) {
      std::cerr << "Error: missing timestamps with producer (" << prod << ")" << std::endl;
      return;
    }
    art::fill_ptr_vector(_raw_timestamps_handle, timestamp_handle);
  }

  // analyze timestamp distributions
  // collect timestamps
  unsigned timeindex = 0;
  for (auto const& timestamps: _raw_timestamps_handle) {
    long this_ts = timestamps->GetTimeStamp();
    ts_vec.push_back(this_ts);
    timeindex++;
  }
  // make binned hist
  long ts_min = ts_vec[0];
  long ts_max = ts_vec[0];
  for (long ts : ts_vec) {
    if (ts < ts_min) {
      ts_min = ts;
    }
    if (ts > ts_max) {
      ts_max = ts;
    }
  }
  float nbin = 100.;
  float bin_width = (ts_max - ts_min)/nbin;
  for (int i = 0; i < 100; ++i) {
    float bin_low = ts_min + i*bin_width;
    float bin_high = ts_min + (i+1)*bin_width;
    int bin_count = 0;
    for (long ts : ts_vec) {
      if ((bin_low < ts) & (ts <= bin_high)) {
        bin_count += 1;
      }
    }
    ts_hist_vec.push_back(bin_count);
  }
  // find peak bins
  for (int bc : ts_hist_vec) {
    //TODO: make threshold fcl parameter
    if (bc > 1000) {
      npeak += 1;
    }
  }

  unsigned index = 0;
  // calculate per channel stuff 
  for (auto const& digits: _raw_digits_handle) {
    // ignore channels over limit
    if (digits->Channel() >= _channel_info.NChannels()) continue;
    _channel_index_map[digits->Channel()] = index;
    ProcessChannel(*digits);
    index++;
  }


  if (_config.timing) {
    _timing.StartTime();
  }
  // make the reduced channel data stuff if need be
  if (_config.reduce_data) {
    for (size_t i = 0; i < _per_channel_data.size(); i++) {
      _per_channel_data_reduced[i] = tpcAnalysis::ReducedChannelDataSBND(_per_channel_data[i]);
    }
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.reduce_data);
  }

  if (_config.timing) {
    _timing.StartTime();
  }

  // now calculate stuff that depends on stuff between channels

  // DNoise
  for (unsigned i = 0; i < _channel_info.NChannels() - 1; i++) {
    unsigned next_channel = i + 1; 

    if (!_per_channel_data[i].empty && !_per_channel_data[next_channel].empty) {
      unsigned raw_digits_i = _channel_index_map[i];
      unsigned raw_digits_next_channel = _channel_index_map[next_channel];
      float unscaled_dnoise = _noise_samples[i].DNoise(
          _raw_digits_handle[raw_digits_i]->ADCs(), _noise_samples[next_channel], _raw_digits_handle[raw_digits_next_channel]->ADCs(), _config.n_max_noise_samples);
      // Don't use same noise sample to scale dnoise
      // This should probably be ok, as long as the dnoise sample is large enough

      // but special case when rms is too small
      if (_per_channel_data[i].rms > 1e-4 && _per_channel_data[next_channel].rms > 1e-4) {
        float dnoise_scale = sqrt(_per_channel_data[i].rms * _per_channel_data[i].rms + 
                                  _per_channel_data[next_channel].rms * _per_channel_data[next_channel].rms);
    
        _per_channel_data[i].next_channel_dnoise = unscaled_dnoise / dnoise_scale; 
      }
      else {
        _per_channel_data[i].next_channel_dnoise = 1.;
      }
    }
  }
  // don't set last dnoise
  _per_channel_data[_channel_info.NChannels() - 1].next_channel_dnoise = 0;

  if (_config.timing) {
    _timing.EndTime(&_timing.coherent_noise_calc);
  }

  if (_config.timing) {
    _timing.StartTime();
  }
  // deal with the header
  if (_config.n_headers > 0) {
    art::InputTag tag1 {"daq"};
    if (auto hdrs = event.getHandle<std::vector<tpcAnalysis::HeaderData>>(tag1)) {
      for (auto const& hdr: *hdrs){
        ProcessHeader(hdr);
      }
    }
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.copy_headers);
  }
  // print stuff out
  if (_config.verbose) {
    std::cout << "EVENT NUMBER: " << _event_ind << std::endl;
    for (auto &channel_data: _per_channel_data) {
      std::cout << channel_data.Print();
    }
  }
  if (_config.timing) {
    _timing.Print();
  }
}


void AnalysisSBND::ProcessHeader(const tpcAnalysis::HeaderData &header) {
  _header_data[header.index] = header;
}
	      
void AnalysisSBND::ProcessChannel(const raw::RawDigit &digits) {
  auto channel = digits.Channel();
  if (channel >= _channel_info.NChannels()) return;

  // don't process the same channel twice
  if (!_per_channel_data[channel].empty) return;

  // handle empty events
  if (digits.NADC() == 0) {
    // default constructor handles empty event
    _per_channel_data[channel] = ChannelDataSBND(channel);
    _noise_samples[channel] = NoiseSample();
    return;
  }

  // if there are ADC's, the channel isn't empty
  _per_channel_data[channel].empty = false;
 
  // re-allocate FFT if necessary
  if (_fft_manager.InputSize() != digits.NADC()) {
    _fft_manager.Set(digits.NADC());
  }
   
  _per_channel_data[channel].channel_no = channel;

  auto adc_vec = digits.ADCs();
  if (_config.timing) {
    _timing.StartTime();
  }
  auto n_adc = digits.NADC();
  if (_config.fill_waveforms || _config.fft_per_channel) {
    for (unsigned i = 0; i < n_adc; i ++) {
      int16_t adc = adc_vec[i];
    
      // fill up waveform
       if (_config.fill_waveforms) {
        _per_channel_data[channel].waveform.push_back(adc);
      }

      if (_config.fft_per_channel) {
        // fill up fftw array
        double *input = _fft_manager.InputAt(i);
        *input = (double) adc;
      }
    }
  }

  if (_config.timing) {
    _timing.EndTime(&_timing.fill_waveform);
  }
  if (_config.timing) {
    _timing.StartTime();
  }
  if (_config.baseline_calc == 0) {
    _per_channel_data[channel].baseline = 0;
  }
  else if (_config.baseline_calc == 1) {
    _per_channel_data[channel].baseline = digits.GetPedestal();
  }
  else if (_config.baseline_calc == 2) {
    _per_channel_data[channel].baseline = Mode(digits.ADCs(), _config.n_mode_skip);
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.baseline_calc);
  }

  if (_config.timing) {
    _timing.StartTime();
  }
  // calculate FFTs
  if (_config.fft_per_channel) {
    _fft_manager.Execute();
    int adc_fft_size = _fft_manager.OutputSize();
    for (int i = 0; i < adc_fft_size; i++) {
      _per_channel_data[channel].fft_real.push_back(_fft_manager.ReOutputAt(i));
      _per_channel_data[channel].fft_imag.push_back(_fft_manager.ImOutputAt(i));
      _per_channel_data[channel].fft_mag.push_back(sqrt(_fft_manager.ReOutputAt(i) * _fft_manager.ReOutputAt(i) + _fft_manager.ImOutputAt(i) * _fft_manager.ImOutputAt(i)));
    } 
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.execute_fft);
  }

  if (_config.timing) {
    _timing.StartTime();
  }
  // get thresholds 
  float threshold = _config.threshold;
  if (_config.threshold_calc == 0) {
    threshold = _config.threshold;
  }
  else if (_config.threshold_calc == 1) {
    auto thresholds = Threshold(adc_vec, _per_channel_data[channel].baseline, _config.threshold_sigma, _config.verbose);
    threshold = thresholds.Val();
  }
  else if (_config.threshold_calc == 2) {
    NoiseSample temp({{0, (unsigned)digits.NADC()-1}}, _per_channel_data[channel].baseline);
    float raw_rms = temp.RMS(adc_vec);
    threshold = raw_rms * _config.threshold_sigma;
  }
  else if (_config.threshold_calc == 3) {
    // if using plane data, make collection planes reach a higher threshold
    float n_sigma = _config.threshold_sigma;
    if (_config.use_planes && _channel_info.PlaneType(channel) == PeakFinder::collection) n_sigma = n_sigma * 1.5;
  
    threshold = _thresholds[channel].Threshold(adc_vec, _per_channel_data[channel].baseline, n_sigma);
  }
  if (_config.timing) {
    _timing.EndTime(&_timing.calc_threshold);
  }

  _per_channel_data[channel].threshold = threshold;

  if (_config.timing) {
    _timing.StartTime();
  }
  // get Peaks

  if (_config.find_signal) {
    PeakFinder::plane_type plane = (_config.use_planes) ? _channel_info.PlaneType(channel) : PeakFinder::unspecified;
  
    PeakFinder peaks(adc_vec, _per_channel_data[channel].baseline, threshold, 
        _config.n_smoothing_samples, _config.n_above_threshold, plane);
    _per_channel_data[channel].peaks.assign(peaks.Peaks()->begin(), peaks.Peaks()->end());
  }

  if (_config.timing) {
    _timing.EndTime(&_timing.find_peaks);
  }

  if (_config.timing) {
    _timing.StartTime();
  }
  // get noise samples
  if (_config.noise_range_sampling == 0) {
    // use first n_noise_samples
    _noise_samples[channel] = NoiseSample( { { 0, _config.n_noise_samples -1 } }, _per_channel_data[channel].baseline);
  }
  else {
    // or use peak finding
    _noise_samples[channel] = NoiseSample(_per_channel_data[channel].peaks, _per_channel_data[channel].baseline, digits.NADC()); 
  }

  // Refine baseline values by taking the mean over the background range
  if (_config.refine_baseline) {
    _noise_samples[channel].ResetBaseline(adc_vec);
    _per_channel_data[channel].baseline = _noise_samples[channel].Baseline(); 
  }

  _per_channel_data[channel].rms = _noise_samples[channel].RMS(adc_vec, _config.n_max_noise_samples);
  _per_channel_data[channel].noise_ranges = *_noise_samples[channel].Ranges();
  if (_config.timing) {
    _timing.EndTime(&_timing.calc_noise);
  }

  // register rms if using running threshold
  if (_config.threshold_calc == 3) {
    _thresholds[channel].AddRMS(_per_channel_data[channel].rms);
  }

  // calculate derived quantities
  _per_channel_data[channel].occupancy = _per_channel_data[channel].Occupancy();
  _per_channel_data[channel].mean_peak_height = _per_channel_data[channel].meanPeakHeight();
}

bool AnalysisSBND::EmptyEvent() {
  return _per_channel_data[0].empty;
}

float AnalysisSBND::Correlation(unsigned channel_i, unsigned channel_j, unsigned max_sample) {
  unsigned digits_i = _channel_index_map[channel_i];
  unsigned digits_j = _channel_index_map[channel_j];
  return _noise_samples[channel_i].Correlation(_raw_digits_handle[digits_i]->ADCs(), 
    _noise_samples[channel_j], _raw_digits_handle[digits_j]->ADCs(), max_sample);
}

std::vector<float> AnalysisSBND::CorrelationMatrix(unsigned max_sample) {
  _timing.StartTime();
  unsigned n_channels = _channel_info.NChannels();
  std::vector<float> ret(n_channels * n_channels, 0);
  for (unsigned i = 0; i < n_channels; i++) {
    for (unsigned j = 0; j <= i; j++) {
      unsigned set = i * n_channels + j;
      unsigned set_diag = j * n_channels + i;
      if (i == j) ret[set] = 1.;
      else {
        float corr = Correlation(i, j, max_sample);
        ret[set] = corr;
        ret[set_diag] = corr;
      }
    }
  }
  float delta = 0.;
  _timing.EndTime(&delta);
  std::cout << "Correlation matrix took: " << delta << " [ms]\n";
  return ret;
}

void TimingSBND::StartTime() {
  start = std::chrono::high_resolution_clock::now(); 
}
void TimingSBND::EndTime(float *field) {
  auto now = std::chrono::high_resolution_clock::now();
  *field += std::chrono::duration<float, std::milli>(now- start).count();
}
void TimingSBND::Print() {
  std::cout << "FILL WAVEFORM: " << fill_waveform << std::endl;
  std::cout << "CALC BASELINE: " << baseline_calc << std::endl;
  std::cout << "FFT   EXECUTE: " << execute_fft << std::endl;
  std::cout << "CALC THRESHOLD " << calc_threshold << std::endl;
  std::cout << "CALC PEAKS   : " << find_peaks << std::endl;
  std::cout << "CALC NOISE   : " << calc_noise << std::endl;
  std::cout << "REDUCE DATA  : " << reduce_data << std::endl;
  std::cout << "COHERENT NOISE " << coherent_noise_calc << std::endl;
  std::cout << "COPY HEADERS : " << copy_headers << std::endl;
}

AnalysisSBND::ChannelInfo::ChannelInfo(const fhicl::ParameterSet &param) {
  // number of channels to be analyzed.
  // Assumes the passed in wire objects will have an wire ID [0, n_channels)
  _n_channels = param.get<unsigned>("n_channels", 0);
  // collection_channels and induction_channels should be a list of channels
  // assigned to the collection or induction plane.

  // For example, the cofiguration:
  // collection_channels: [ [1,2] , [15, 25] ]
  // will set channels 1,15,16,17,18,19,20,21,22,23,24 to be collection channels

  // if a channel is set to neither collection nor induction, then it will be 
  // set as "unspecified" (see PeakFinder for behavior of peak finding on unspecified channels)
  std::vector<std::vector<unsigned>> collection_lists = param.get<std::vector<std::vector<unsigned>>>("collection_channels", { {} });
  for (const std::vector<unsigned> &channel_pair: collection_lists) {
    for (unsigned i = channel_pair[0]; i < channel_pair[1]; i++) {
      _collection_channels.insert(i);
    }
  }

  std::vector<std::vector<unsigned>> induction_lists = param.get<std::vector<std::vector<unsigned>>>("induction_channels", { {} });
  for (const std::vector<unsigned> &channel_pair: induction_lists) {
    for (unsigned i = channel_pair[0]; i < channel_pair[1]; i++) {
      _induction_channels.insert(i);
    }
  }
  
}

unsigned AnalysisSBND::ChannelInfo::NChannels() { return _n_channels; }

PeakFinder::plane_type AnalysisSBND::ChannelInfo::PlaneType(unsigned channel) {
  bool is_collection = _collection_channels.find(channel) != _collection_channels.end();
  if (is_collection) return PeakFinder::collection;

  bool is_induction = _induction_channels.find(channel) != _induction_channels.end();
  if (is_induction) return PeakFinder::induction;

  return PeakFinder::unspecified;
}

