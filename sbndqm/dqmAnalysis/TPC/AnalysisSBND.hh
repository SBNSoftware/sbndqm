#ifndef Analysis_hh
#define Analysis_hh

#include <vector>
#include <string>
#include <ctime>
#include <chrono>
#include <numeric>

#include "TROOT.h"
#include "TTree.h"

#include "art/Framework/Core/EDAnalyzer.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/RDTimeStamp.h"
#include "lardataobj/RecoBase/Hit.h"
#include "larcore/Geometry/Geometry.h"

#include "ChannelDataSBND.hh"
#include "sbndqm/Decode/TPC/HeaderData.hh"
#include "FFT.hh"
#include "Noise.hh"

/*
  * Main analysis code of the online Monitoring.
  * Takes as input raw::RawDigits and produces
  * a number of useful metrics all defined in 
  * ChannelDataSBND.hh
*/

namespace tpcAnalysis {
  class AnalysisSBND;
  class TimingSBND;
}

// keep track of timing information
class tpcAnalysis::TimingSBND {
public:
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  float fill_waveform;
  float baseline_calc;
  float execute_fft;
  float calc_threshold;
  float find_peaks;
  float calc_noise;
  float reduce_data;
  float coherent_noise_calc;
  float copy_headers;

  TimingSBND():
    fill_waveform(0),
    baseline_calc(0),
    execute_fft(0),
    calc_threshold(0),
    find_peaks(0),
    calc_noise(0),
    reduce_data(0),
    coherent_noise_calc(0),
    copy_headers(0)
  {}
  
  void StartTime();
  void EndTime(float *field);

  void Print();
};


class tpcAnalysis::AnalysisSBND {
public:
  explicit AnalysisSBND(fhicl::ParameterSet const & p);

  // actually analyze stuff
  void AnalyzeEvent(art::Event const & e);

  // calculate the correlation between two channels
  // Call after AnalyzeEvent()
  float Correlation(unsigned channel_i, unsigned channel_j, unsigned max_sample=UINT_MAX);
  // and build the whole matrix
  std::vector<float> CorrelationMatrix(unsigned max_sample=UINT_MAX);

  // configuration
  struct AnalysisSBNDConfig {
    public:
    std::string output_file_name;
    bool verbose;
    std::vector<std::string> producers;
    std::string instance;
    std::string header_producer;
    int static_input_size;

    int n_headers;
    float threshold;
    float threshold_sigma;
    
    unsigned baseline_calc;
    bool refine_baseline;
    unsigned n_mode_skip;
    unsigned noise_range_sampling;
    bool use_planes;
    unsigned threshold_calc;
    unsigned n_noise_samples;
    unsigned n_smoothing_samples;
    unsigned n_above_threshold;
    unsigned n_max_noise_samples;
    bool find_signal;

    bool fft_per_channel;
    bool fill_waveforms;
    bool reduce_data;
    bool timing;

    AnalysisSBNDConfig(const fhicl::ParameterSet &param);
    AnalysisSBNDConfig() {}
  };


  // holds limited channel information
  class ChannelInfo {
    public:
      explicit ChannelInfo(const fhicl::ParameterSet &param);
      unsigned NChannels(); //!< Total number of channels to be analyzed
      PeakFinder::plane_type PlaneType(unsigned channel); //!< plane associated with this channel

    private:
      unsigned _n_channels;
      std::set<unsigned> _collection_channels;
      std::set<unsigned> _induction_channels;
  };

  // other functions
  void ProcessChannel(const raw::RawDigit &digits);
  void ProcessHeader(const tpcAnalysis::HeaderData &header);

  // if the containers filled by the analysis are ready to be processed
  bool EmptyEvent();

public:
  ChannelInfo _channel_info; //!< Information about TPC channels
  // configuration is available publicly
  AnalysisSBNDConfig _config;
  // keeping track of wire id to index into stuff from Decoder
  std::vector<unsigned> _channel_index_map;
  // output containers of analysis code. Only use after calling ReadyToProcess()
  std::vector<tpcAnalysis::ChannelDataSBND> _per_channel_data;
  std::vector<tpcAnalysis::ReducedChannelDataSBND> _per_channel_data_reduced;
  int npeak;
  std::vector<tpcAnalysis::NoiseSample> _noise_samples;
  std::vector<tpcAnalysis::HeaderData> _header_data;
  std::vector<RunningThreshold> _thresholds;
  // raw digits container for each event
  std::vector<art::Ptr<raw::RawDigit>> _raw_digits_handle;
  std::vector<art::Ptr<raw::RDTimeStamp>> _raw_timestamps_handle;
  FFTManager _fft_manager;

private:
  unsigned _event_ind;
  // keep track of timing data (maybe)
  tpcAnalysis::TimingSBND _timing;

  // header data container for each event
  art::Handle<std::vector<tpcAnalysis::HeaderData>> _header_data_handle;
};



#endif /* Analysis_hh */
