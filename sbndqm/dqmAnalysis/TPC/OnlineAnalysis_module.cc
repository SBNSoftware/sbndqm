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
  void SendSparseWaveforms();
  void SendWaveforms();
  void SendFFTs();
  tpcAnalysis::Analysis _analysis;
  double _tick_period;
  bool _send_sparse_waveforms;
  bool _send_waveforms;
  bool _send_ffts;
  bool _send_metrics;
  int _wait_period;
  int _last_time;
  std::string fFFTName;
  std::string fWaveformName;
  std::string fGroupName;
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
  _send_metrics = p.get<bool>("send_metrics", true);
  _wait_period = p.get<int>("wait_period", -1);
  _last_time = -1;

  fFFTName = p.get<std::string>("fft_name", "fft");
  fGroupName = p.get<std::string>("group_name", "tpc_channel");
  fWaveformName = p.get<std::string>("waveform_name", "waveform");

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

  _analysis.AnalyzeEvent(e);
  // calculate correlations here if you want to:
  // e.g. _analysis.Correlation(channel_i, channel_j);
  // or calculate the whole matrix:
  // _analysis.CorrelationMatrix()

  // Save zero-suppressed waveforms -- use hitfinding to determine interesting regions
  if (_send_sparse_waveforms) SendSparseWaveforms();
 
  if (_send_waveforms) SendWaveforms();
  
  if (_send_ffts) SendFFTs();

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

void tpcAnalysis::OnlineAnalysis::SendWaveforms() {
  for (auto const& digits: _analysis._raw_digits_handle) {
    const std::vector<int16_t> &adcs = digits->ADCs();
     std::string redis_key = "snapshot:" + fWaveformName + ":wire:" + std::to_string(digits->Channel());
     sbndaq::SendWaveform(redis_key, adcs, 0.4 /* tick period in us */);
  }
}

void tpcAnalysis::OnlineAnalysis::SendFFTs() {
  for (const ChannelData &chan: _analysis._per_channel_data) {
    if (chan.fft_mag.size()) {
      std::string redis_key = "snapshot:"+ fFFTName + ":wire:" + std::to_string(chan.channel_no);
      sbndaq::SendWaveform(redis_key, chan.fft_mag, 2.5 /* tick freq. in MHz */);
    }
  }
}

void tpcAnalysis::OnlineAnalysis::SendSparseWaveforms() {
  // use analysis hit-finding to determine interesting regions of hits
  for (unsigned channel = 0; channel < _analysis._per_channel_data.size(); channel++) {
    const ChannelData &data = _analysis._per_channel_data[channel];
    std::vector<std::vector<int16_t>> sparse_waveforms;
    std::vector<float> offsets;

    // empty channel -- jsut reset the waveform and continue
    if (data.empty) {
      std::string key = "snapshot:sparse_waveform:wire:" + std::to_string(channel);
      sbndaq::SendSplitWaveform(key, sparse_waveforms, offsets, _tick_period);
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
  } 
}

DEFINE_ART_MODULE(tpcAnalysis::OnlineAnalysis)
