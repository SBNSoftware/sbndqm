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
  InitializeMetricManager(p.get<fhicl::ParameterSet>("metrics"));
}

std::string keyName(const char *metric, unsigned channel_no) {
  return "channel:" + std::to_string(channel_no) + ":" + metric;
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
    std::string name;

    name = keyName("rms", channel_data.channel_no);
    value = channel_data.rms;
    sendMetric(name.c_str(), value, level, mode);

    name = keyName("baseline", channel_data.channel_no);
    value = channel_data.baseline;
    sendMetric(name.c_str(), value, level, mode);

    name = keyName("next_channel_dnoise", channel_data.channel_no);
    value = channel_data.next_channel_dnoise;
    sendMetric(name.c_str(), value, level, mode);

    name = keyName("mean_peak_height", channel_data.channel_no);
    value = channel_data.mean_peak_height;
    sendMetric(name.c_str(), value, level, mode);

    name = keyName("occupancy", channel_data.channel_no);
    value = channel_data.occupancy;
    sendMetric(name.c_str(), value, level, mode);
  }
}


DEFINE_ART_MODULE(tpcAnalysis::OnlineAnalysis)
