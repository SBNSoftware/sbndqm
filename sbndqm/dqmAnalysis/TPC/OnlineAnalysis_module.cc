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
#include "art/Framework/Services/Optional/TFileService.h"

#include "ChannelData.hh"
#include "HeaderData.hh"
#include "Analysis.hh"

#include "../../MetricManagerShim/MetricManager.hh"
#include "../../MetricConfig/ConfigureRedis.hh"

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
  tpcAnalysis::Analysis _analysis;
};

tpcAnalysis::OnlineAnalysis::OnlineAnalysis(fhicl::ParameterSet const & p):
  art::EDAnalyzer::EDAnalyzer(p),
  _analysis(p)
{
  sbndqm::InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
  sbndqm::GenerateMetricConfig(p.get<fhicl::ParameterSet>("metric_config"));
}

void tpcAnalysis::OnlineAnalysis::analyze(art::Event const & e) {
  _analysis.AnalyzeEvent(e);
  // calculate correlations here if you want to:
  // e.g. _analysis.Correlation(channel_i, channel_j);
  // or calculate the whole matrix:
  // _analysis.CorrelationMatrix()

  int level = 0;
  artdaq::MetricMode mode = artdaq::MetricMode::Average;
  // send the metrics
  for (auto const &channel_data: _analysis._per_channel_data) {
    double value;
    std::string instance = std::to_string(channel_data.channel_no);

    value = channel_data.rms;
    sbndqm::sendMetric("tpc_channel", instance, "rms", value, level, mode);

    value = channel_data.baseline;
    sbndqm::sendMetric("tpc_channel", instance, "baseline", value, level, mode);

    value = channel_data.next_channel_dnoise;
    sbndqm::sendMetric("tpc_channel", instance, "next_channel_dnoise", value, level, mode);

    value = channel_data.mean_peak_height;
    sbndqm::sendMetric("tpc_channel", instance, "mean_peak_height", value, level, mode);

    value = channel_data.occupancy;
    sbndqm::sendMetric("tpc_channel", instance, "occupancy", value, level, mode);
  }
}


DEFINE_ART_MODULE(tpcAnalysis::OnlineAnalysis)
