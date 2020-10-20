***DaqAnalysis***

Code for online analysis of TPC data for use by ICARUS and SBN. The code
takes as input raw data files in the art-ROOT format and sends
monitoring metrics and raw data of individual TPC channels to a
database. 

Metrics that the online analysis calculates:
baseline: The baseline/pedestal ADC value of the waveform
rms: The rms of the noise of the waveform (signal-remvoed)
occupancy: The mean number of hits per readout
mean_peak_amplitude: The mean amplitude of each hit
next_channel_dnoise: A measure of noise correlation between subsequent channels

The online analysis also stores a 'snapshot' of the detector every few
events. This snapshot includes:
-Noise correlation (signal-removed) of each pair of channels
-Waveform of each channel
-FFT of each channel
-Waveform of each channel averaged over previous events
-FFT of each channel averaged over previous events

The code is organized into three art modules:

- `DaqDecoder` (a producer) takes in an art root file with
  NevisTPCFragments and produces and art root file including rawDigits
  (useful for e.g. RawHitFinder). Configuration options:
   - produce_header (bool): whether to also produce a HeaderData (one
     per FEM) object for each fragment read in
   - calc_baseline (bool): whether to calculate the pedestal. The
     resulting value is stored in the RawDigits but is __not__
     subtracted.
   - n_mode_skip (unsigned): set the percentage of ADC values considered
     in mode finding to be (100 / n_mode_skip)
   - validate_header (bool): whether to run validation on the headers.
     Errors are printed through MessageService.
   - calc_checksum (bool): whether to calculate the checksum to check
     against the firmware checksum.
- `OfflineAnalysis` and `OnlineAnalysis` (both analyzers) are both wrappers
  around the Analysis class. The Analysis class taked the RawDigits as
  input. `OfflineAnalysis` writes to a file and `OnlineAnalysis` writes to a 
  redis database.  
- Analysis options, used by both modules:
  - verbose (bool): whether to print out information on analysis (Note:
    this is option is __very__ verbose).
  - threshold_calc (unsigned): Method for calculating thresholds for
    peak finding. Options:
    - 0: use static thresholds set in threshold fcl parameter
    - 1: use gauss fitter to calculate rms, then scale by
      threshold_sigma fcl parameter
    - 2: use raw rms of waveform, scaled by threchold_sigma
    - 3: use rolling average of past rms values, scaled by
      threshold_sigma
  - n_above_threshold (unsigned): number of consecutive ADC samples
    above threshold required before declaring a peak
  - noise_range_sampling (unsigned): Method for determining ranges for
    calculating noise RMS (and other) values. Options:
    - 0: assume the first n_noise_samples (a fcl parameter) represent
      a signal-free sample
    - 1: use peakfinding to exclude signal regions
  - baseline_calc (unsigned): Method for determining pedestal. Options:
    - 0: assume the pedestal in each channel is 0.
    - 1: assume pedestal is set in RawDigits (i.e. by DaqDecoder)
    - 2: use mode finding to calculate baseline
  - refine_baseline (bool): Whether to recalculate the pedestal after
    peak finding by taking the mean of all noise samples. Will produce a
    more precise baseline (especially in the presence of large frequency
    noise) at the cost of some extra calculation time.
  - n_mode_skip (unsigned): Set the percentage of ADC values considered
    in mode/pedestal finding to be (100 / n_mode_skip)
  - static_input_size (unsigned): Number of ADC counts in waveform. If
    set, will marginally speed up FFT calculations.
  - n_headers (unsigned): Number of headers to be analyzed. If not set,
    code will not analyze header info.
  - sum_waveforms (bool): Whether to sum all waveforms across FEM's.
  - fft_per_channel (bool): Whether to calculate an FFT on each channel
    waveform.
  - reduce_data (bool): Whether to write ReducedChannelData to disk
    instead of ChannelData (will produce smaller sized files).
  - timing (bool): Whether to print out timing info on analysis.
  - producer (string): Name of digits producer
- `OnlineAnalysis` options:
  - metric_config: sets up the metric configuration
  - metrics: sets up the metric streams to the database
- `VSTAnalysis` options:
  - no additional options

**Other Configuration Notes**

A group of sensible default parameters is set in
simple_pipeline.fcl and analysis.fcl (for `VSTAnalysis`) and 
redis_pipeline.fcl (for `OnlineAnalysis`)

