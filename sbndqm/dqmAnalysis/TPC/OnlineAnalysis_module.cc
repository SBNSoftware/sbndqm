#include <vector>
#include <chrono>

#include "TROOT.h"
#include "TTree.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"

#include "canvas/Utilities/InputTag.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h" 
#include "art/Framework/Principal/SubRun.h" 
#include "art_root_io/TFileService.h"

#include "ChannelData.hh"
#include "sbndaq-decode/TPC/HeaderData.hh"
#include "Analysis.hh"

#include "sbndaq-online/helpers/SBNMetricManager.h"
#include "sbndaq-online/helpers/MetricConfig.h"
#include "sbndaq-online/helpers/Waveform.h"
#include "sbndaq-online/helpers/Utilities.h"
#include "sbndaq-online/helpers/EventMeta.h"

/*
 * Uses the Analysis class to print stuff to file
*/

namespace tpcAnalysis {
  class OnlineAnalysis;
}


class tpcAnalysis::OnlineAnalysis : public art::EDAnalyzer {
public:
  explicit OnlineAnalysis(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OnlineAnalysis(OnlineAnalysis const &) = delete;
  OnlineAnalysis(OnlineAnalysis &&) = delete;
  OnlineAnalysis & operator = (OnlineAnalysis const &) = delete;
  OnlineAnalysis & operator = (OnlineAnalysis &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;
private:
  void SendSparseWaveforms(const art::Event &e);
  void SendWaveforms(const art::Event &e);
  void SendFFTs(const art::Event &e);
  void SendTimeAvgFFTs(const art::Event &e);
  void SendCorrelationMatrix(const art::Event &e);

  tpcAnalysis::Analysis _analysis;
  double _tick_period;
  bool _send_sparse_waveforms;
  bool _send_waveforms;
  bool _send_ffts;
  bool _send_time_avg_ffts;
  bool _send_correlation_matrix;
  int _n_evt_fft_avg;
  bool _send_metrics;
  int _wait_period;
  int _last_time;
  std::string fFFTName;
  std::string fWaveformName;
  std::string fGroupName;
  std::string fAvgFFTName;
  std::string fAvgWvfName;
  std::string fCorrelationMatrixName;
  unsigned fNCorrelationMatrixSamples;

  // fields used to compute time averaged FFT's
  class {
  public:
    std::vector<std::vector<double>> waveforms;
    int event_ind;
    bool first;
  } fAvgFFTData;

  // keep track of sending raw data
  unsigned _n_evt_send_rawdata;
  unsigned event_ind;
};

tpcAnalysis::OnlineAnalysis::OnlineAnalysis(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p)
{
  if (p.has_key("metrics")) {
    sbndaq::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
  }
  if (p.has_key("metric_config")) {
    sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_config"));
  }
  _tick_period = p.get<double>("tick_period", 500 /* ns */);
  _send_sparse_waveforms = p.get<bool>("send_sparse_waveforms", false);
  _send_waveforms = p.get<bool>("send_waveforms", false);
  _send_ffts = p.get<bool>("send_ffts", false);
  _send_time_avg_ffts = p.get<bool>("send_time_avg_ffts", false);
  _n_evt_fft_avg = p.get<unsigned>("n_evt_fft_avg", 1);
  assert(_n_evt_fft_avg > 0);
  _send_metrics = p.get<bool>("send_metrics", true);
  _wait_period = p.get<int>("wait_period", -1);
  _last_time = -1;
  _n_evt_send_rawdata = p.get<unsigned>("n_evt_send_rawdata", 1);
  assert(_n_evt_send_rawdata > 1);

  _send_correlation_matrix = p.get<bool>("send_correlation_matrix", false);

  fAvgFFTData.first = true;

  fFFTName = p.get<std::string>("fft_name", "fft");
  fGroupName = p.get<std::string>("group_name", "tpc_channel");
  fWaveformName = p.get<std::string>("waveform_name", "waveform");
  fAvgFFTName = p.get<std::string>("avgfft_name", "avgfft");
  fAvgWvfName = p.get<std::string>("avgwvf_name", "avgwvf");
  fCorrelationMatrixName = p.get<std::string>("correlation_matrix_name", "correlation");
  fNCorrelationMatrixSamples = p.get<unsigned>("n_correlation_matrix_samples", UINT_MAX);

  event_ind = 0;
}

void tpcAnalysis::OnlineAnalysis::analyze(art::Event const & e) {
  // UNIX time in ms
  int this_time = std::chrono::duration_cast< std::chrono::seconds >(
    std::chrono::system_clock::now().time_since_epoch()
    ).count();
  // if we are configured to, don't run on this event
  if (_wait_period > 0. && _last_time > 0. && (this_time - _last_time) < _wait_period) {
    return;
  }
  _last_time = this_time;

  event_ind ++;

  _analysis.AnalyzeEvent(e);

  if (_send_correlation_matrix  && event_ind % _n_evt_send_rawdata == 0) SendCorrelationMatrix(e);

  // Save zero-suppressed waveforms -- use hitfinding to determine interesting regions
  if (_send_sparse_waveforms) SendSparseWaveforms(e);
 
  if (_send_waveforms && event_ind % _n_evt_send_rawdata == 0) SendWaveforms(e);
  
  if (_send_ffts && event_ind % _n_evt_send_rawdata == 0) SendFFTs(e);

  if (_send_time_avg_ffts) SendTimeAvgFFTs(e);

  // Save metrics
  if (_send_metrics) {
    int level = 0;
    artdaq::MetricMode mode = artdaq::MetricMode::Average;
    // send the metrics
    for (auto const &channel_data: _analysis._per_channel_data) {
      double value;
      std::string instance = std::to_string(channel_data.channel_no);
      
      value = channel_data.rms;
      sbndaq::sendMetric(fGroupName, instance, "rms", value, level, mode);

      value = channel_data.baseline;
      sbndaq::sendMetric(fGroupName, instance, "baseline", value, level, mode);

      value = channel_data.next_channel_dnoise;
      sbndaq::sendMetric(fGroupName, instance, "next_channel_dnoise", value, level, mode);

      value = channel_data.mean_peak_height;
      sbndaq::sendMetric(fGroupName, instance, "mean_peak_height", value, level, mode);

      value = channel_data.occupancy;
      sbndaq::sendMetric(fGroupName, instance, "occupancy", value, level, mode);
    }
  }
}

void tpcAnalysis::OnlineAnalysis::SendCorrelationMatrix(const art::Event &e) {
  std::vector<float> matrix = _analysis.CorrelationMatrix(fNCorrelationMatrixSamples);
  std::string redis_key = "snapshot:" + fCorrelationMatrixName;
  sbndaq::SendWaveform(redis_key, matrix);
  sbndaq::SendEventMeta(redis_key, e);
}

void tpcAnalysis::OnlineAnalysis::SendWaveforms(const art::Event &e) {
  for (auto const& digits: _analysis._raw_digits_handle) {
    const std::vector<int16_t> &adcs = digits->ADCs();
     std::string redis_key = "snapshot:" + fWaveformName + ":wire:" + std::to_string(digits->Channel());
     sbndaq::SendWaveform(redis_key, adcs, 0.4 /* tick period in us */);
     sbndaq::SendEventMeta(redis_key, e); 
  }
}

void tpcAnalysis::OnlineAnalysis::SendTimeAvgFFTs(const art::Event &e) {

  // first time setup -- set the size of each waveform
  if (fAvgFFTData.first) {
    // set the waveform size to the number of channels
    std::fill_n(std::back_inserter(fAvgFFTData.waveforms), _analysis._channel_info.NChannels(), std::vector<double>());
    fAvgFFTData.event_ind = 0;

    unsigned n_ticks = _analysis._raw_digits_handle[0]->NADC();
    for (unsigned i = 0; i < fAvgFFTData.waveforms.size(); i++) {
      std::fill_n(std::back_inserter(fAvgFFTData.waveforms[i]), n_ticks, 0.);
    }
    fAvgFFTData.first = false;
  }
  
  if (fAvgFFTData.event_ind == _n_evt_fft_avg) {
    // Do send
    for (unsigned i = 0; i < fAvgFFTData.waveforms.size(); i++) {
      std::vector<float> fft;

      if (_analysis._fft_manager.InputSize() != fAvgFFTData.waveforms[i].size()) {
        _analysis._fft_manager.Set(fAvgFFTData.waveforms[i].size());
      }

      for (unsigned ind = 0; ind < fAvgFFTData.waveforms[i].size(); ind++) {
        // fill up the FFT array
        double *input = _analysis._fft_manager.InputAt(ind);
        *input = fAvgFFTData.waveforms[i][ind];
      }
      // Execute the FFT
      _analysis._fft_manager.Execute();
      int adc_fft_size = _analysis._fft_manager.OutputSize();
      // save the data
      for (int i = 0; i < adc_fft_size; i++) {
        fft.push_back( 
          sqrt(_analysis._fft_manager.ReOutputAt(i) * _analysis._fft_manager.ReOutputAt(i) + 
               _analysis._fft_manager.ImOutputAt(i) * _analysis._fft_manager.ImOutputAt(i)) );
      }

      // send it out
      std::string redis_key = "snapshot:" + fAvgFFTName + ":wire:" + std::to_string(i);
      sbndaq::SendWaveform(redis_key, fft, 2.5 /* tick freq. in MHz */);
      sbndaq::SendEventMeta(redis_key, e); 

      std::string redis_key_wvf = "snapshot:" + fAvgWvfName + ":wire:" + std::to_string(i);
      sbndaq::SendWaveform(redis_key_wvf, fAvgFFTData.waveforms[i], _tick_period); 
      sbndaq::SendEventMeta(redis_key_wvf, e);
    }
    fAvgFFTData.event_ind = 0;
  }

  // Do avg
  for (auto const& digits: _analysis._raw_digits_handle) {
    if (digits->Channel() >= fAvgFFTData.waveforms.size()) continue;

    const std::vector<int16_t> &adcs = digits->ADCs();
    std::vector<double> &wvf = fAvgFFTData.waveforms.at(digits->Channel());
    for (unsigned i = 0; i < wvf.size(); i++) {
      wvf[i] = ((double)adcs.at(i) + wvf[i] * fAvgFFTData.event_ind) / (fAvgFFTData.event_ind+1);
    } 
  }
  fAvgFFTData.event_ind ++; 

}

void tpcAnalysis::OnlineAnalysis::SendFFTs(const art::Event &e) {
  for (const ChannelData &chan: _analysis._per_channel_data) {
    if (chan.fft_mag.size()) {
      std::string redis_key = "snapshot:"+ fFFTName + ":wire:" + std::to_string(chan.channel_no);
      sbndaq::SendWaveform(redis_key, chan.fft_mag, 2.5 /* tick freq. in MHz */);
      sbndaq::SendEventMeta(redis_key, e); 
    }
  }
}

void tpcAnalysis::OnlineAnalysis::SendSparseWaveforms(const art::Event &e) {
  // use analysis hit-finding to determine interesting regions of hits
  for (unsigned channel = 0; channel < _analysis._per_channel_data.size(); channel++) {
    const ChannelData &data = _analysis._per_channel_data[channel];
    std::vector<std::vector<int16_t>> sparse_waveforms;
    std::vector<float> offsets;

    // empty channel -- jsut reset the waveform and continue
    if (data.empty) {
      std::string key = "snapshot:sparse_waveform:wire:" + std::to_string(channel);
      sbndaq::SendSplitWaveform(key, sparse_waveforms, offsets, _tick_period);
      sbndaq::SendEventMeta(key, e); 
      continue;
    }

    const std::vector<int16_t> &adcs = _analysis._raw_digits_handle[_analysis._channel_index_map[data.channel_no]]->ADCs();

    for (unsigned i = 0; i < data.peaks.size(); i++) {
      // don't make a new waveform if this peak is adjacent to the last one
      if (i > 0 && data.peaks[i].start_loose <= data.peaks[i-1].end_loose - 1) {
        size_t waveform_ind = sparse_waveforms.size()-1;
        size_t start_size = sparse_waveforms[waveform_ind].size();

        sparse_waveforms[waveform_ind].insert(
          sparse_waveforms[waveform_ind].end(), 
          adcs.begin() + data.peaks[i-1].end_loose, adcs.begin() + data.peaks[i].end_loose);

        // baseline subtract
        for (unsigned j = start_size; j < sparse_waveforms[waveform_ind].size(); j++) {
           sparse_waveforms[waveform_ind][j] -= data.baseline;
        }
      }
      else {
        const PeakFinder::Peak &peak = data.peaks[i];
        sparse_waveforms.emplace_back(adcs.begin() + peak.start_loose, adcs.begin() + peak.end_loose);

        size_t waveform_ind = sparse_waveforms.size()-1;
        // baseline subtract
        for (unsigned j = 0; j < sparse_waveforms[waveform_ind].size(); j++) {
          sparse_waveforms[waveform_ind][j] -= data.baseline; 
        } 
        offsets.push_back(peak.start_loose); // * _tick_period);
      }
    }
    std::string key = "snapshot:sparse_waveform:wire:" + std::to_string(data.channel_no);
    sbndaq::SendSplitWaveform(key, sparse_waveforms, offsets, _tick_period); 
    sbndaq::SendEventMeta(key, e); 
  } 
}

DEFINE_ART_MODULE(tpcAnalysis::OnlineAnalysis)
