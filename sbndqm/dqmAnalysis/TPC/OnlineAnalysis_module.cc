#include <vector>

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
  tpcAnalysis::Analysis _analysis;
  double _tick_period;
  bool _send_sparse_waveforms;
};

tpcAnalysis::OnlineAnalysis::OnlineAnalysis(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p)
{
  sbndaq::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
  sbndaq::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_config"));
  _tick_period = p.get<double>("tick_period", 500 /* ns */);
  _send_sparse_waveforms = p.get<bool>("send_sparse_waveforms", false);
}

void tpcAnalysis::OnlineAnalysis::analyze(art::Event const & e) {
  _analysis.AnalyzeEvent(e);
  // calculate correlations here if you want to:
  // e.g. _analysis.Correlation(channel_i, channel_j);
  // or calculate the whole matrix:
  // _analysis.CorrelationMatrix()

  // Save zero-suppressed waveforms -- use hitfinding to determine interesting regions
  if (_send_sparse_waveforms) SendSparseWaveforms();

  // Save metrics
  int level = 0;
  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  // send the metrics
  for (auto const &channel_data: _analysis._per_channel_data) {
    double value;
    std::string instance = std::to_string(channel_data.channel_no);

    value = channel_data.rms;
    sbndaq::sendMetric("tpc_channel", instance, "rms", value, level, mode);

    value = channel_data.baseline;
    sbndaq::sendMetric("tpc_channel", instance, "baseline", value, level, mode);

    value = channel_data.next_channel_dnoise;
    sbndaq::sendMetric("tpc_channel", instance, "next_channel_dnoise", value, level, mode);

    value = channel_data.mean_peak_height;
    sbndaq::sendMetric("tpc_channel", instance, "mean_peak_height", value, level, mode);

    value = channel_data.occupancy;
    sbndaq::sendMetric("tpc_channel", instance, "occupancy", value, level, mode);
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

    // const std::vector<int16_t> &adcs = _analysis._raw_digits_handle.at(_analysis._channel_index_map[data.channel_no])->ADCs();
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
