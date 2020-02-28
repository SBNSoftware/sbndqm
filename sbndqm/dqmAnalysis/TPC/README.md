***DaqAnalysis***

Code for online analysis of data acquisition electronics for the
Vertical Slice Test (VST). The code is organized into three art 
modules:

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
- `VSTAnalysis` and `OnlineAnalysis` (both analyzers) are both wrappers
  around the Analysis class. The Analysis class taked the RawDigits as
  input. `VSTAnalysis` writes to a file and `OnlineAnalysis` writes to a 
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
- `VSTAnalysis` options:
  - no additional options

**Other Configuration Notes**

A group of sensible default parameters is set in
simple_pipeline.fcl and analysis.fcl (for `VSTAnalysis`) and 
redis_pipeline.fcl (for `OnlineAnalysis`)

**Plotting Output**

I have a few scripts at https://github.com/gputnam/VSTmonitoring which
can plot the output of the `VSTAnalysis` module. See the README on that
page for documentation.

**Building**

INSTRUCTIONS FOR SETTING UP THE ONLINE ANALYSIS CODE INSIDE OF SBNDCODE:

-Setup Environment:
source /grid/fermiapp/products/sbnd/setup_sbnd.sh
setup mrb

-Setup the MRB Project:
mkdir `your_new_directory`
cd `your_new_directory`
export MRB_PROJECT=ARTDAQ
mrb newDev -v v2_03_03 -q e14:prof:nu:eth:s50
source localProducts_ARTDAQ_v2_03_03_e14_prof_nu_eth_s50/setup
cd srcs

-Install artdaq:
mrb g artdaq
-Go to v2_03_03
cd artdaq
git checkout v2_03_03
-Edit artdaq/ups/product_deps to change a row in the "qualifier list":
-Change the row:
nu:e14:s50:eth:prof     nu:e14:prof     nu:e14:s50:prof    e14:s50:prof         -               e14:prof        e14:prof        e14:g371:s50:prof       e14:s50:prof            e14:s50:prof
-To:
nu:e14:s50:eth:prof     nu:e14:prof     nu:e14:s50:eth:prof    e14:s50:prof             -               e14:prof        e14:prof        e14:g371:s50:prof       e14:s50:prof            e17:s50:prof

-Install artdaq_core
cd .. (out of artdaq)
mrb g artdaq_core
-go to v3_00_00
cd artdaq_core
git checkout v3_00_00
-Edit artdaq_core/ups/product_deps to add a row to the "qualifier list":
-Under the row:
qualifier               canvas          TRACE   notes
-Add:
e14:prof:nu:eth:s50     nu:e14:prof     -nq-    -std=c++14
-With tabs in between each column

Make sure that everything thus far works:
mrbslp (setup local products)
mrbsetenv (set environment)
mrb i -j8 (build)

Install sbndcode:
mrb g sbndcode
-Go to my feature branch
-First add the "redmine" remote so you can pull it
git remote add redmine ssh://p-sbndcode@cdcvs.fnal.gov/cvs/projects/sbndcode
-Now check it out (without merging it onto your current `develop` branch)
git fetch redmine feature/gray_OnlineAnalysis
git checkout redmine/feature/gray_OnlineAnalysis
git checkout -b feature/your_feature_branch_name

-Edit sbndcode/ups/product_deps:
-Under the row:
qualifier        larsoft       sbndutil    sbnd_data  notes
-Add:
e14:prof:nu:eth:s50 e14:prof e14:prof -nq-
-With tabs in between each column
-Also change:
cetbuildtools v5_09_01
-To:
cetbuildtools v5_08_01

-DaqAnalysis code also uses the hiredis library

-on the sbnddaq machines:
-hiredis is built by Andy, the Cmake file in DaqAnalysis is setup to look for it in $HIREDIS_LIB
-so run the below to hook it up:
export HIREDIS_INC=/home/nfs/mastbaum/sw/hiredis/include
export HIREDIS_LIB=/home/nfs/mastbaum/sw/hiredis/lib

-on a different machine:
-clone the hiredis repository from https://github.com/redis/hiredis.git
-`make` in the git directory
-set the HIREDIS_INC and HIREDIS_LIB variables to the proper locations

Install sbnddaq-datatypes:
mrb g sbnddaq-datatypes
-Edit sbnddaq-datatypes/product_deps/ups:
-Under the row:
qualifier               artdaq_core
-Add:
e14:prof:nu:eth:s50     e14:prof:nu:eth:s50

-You __MUST__ build stuff before installing sbnddaq-datatypes (also works as a check)
-Log out, log in, re-setup, then run:
mrbslp
mrbsetenv
mrb i -j8
-If you get:
INFO: no optional setup of sbndutil v01_17_00 -q +e14:+prof
-Then that's ok

Install sbnddaq-readout:
mrb g sbnddaq-readout
-Edit sbnddaq-readout/ups/product_deps:
-Under the row:
qualifier     artdaq             sbnddaq_datatypes caencomm caenvme     caendigitizer   notes
-Add:
e14:prof:nu:eth:s50  e14:prof:nu:eth:s50    e14:prof:nu:eth:s50 -nq-    -nq-    -nq-

Build:
mrbsetenv
mrb i -j8

-Now you're done!
-The following setup script will work whenever you log in going forward:
source /grid/fermiapp/products/sbnd/setup_sbnd.sh
setup mrb
source localProducts_ARTDAQ_v2_03_03_e14_prof_nu_eth_s50/setup
export HIREDIS_INC=/home/nfs/mastbaum/sw/hiredis/include
export HIREDIS_LIB=/home/nfs/mastbaum/sw/hiredis/lib
mrbslp
mrbsetenv

**Setting up Front End Website**

If you are so inclined, you can also set up the front end website with these instructions.

-Get the git repository:
git clone https://github.com/gputnam/minargon.git
-Create a virtualenv for the python depedencies
-For some reason the global virtualenv setup doesn't seem to work (I think because it is outdated) so
I instead used the virtualenv in andy's home directory:
~mastbaum/sw/virtualenv-15.1.0/virtualenv.py env
-Now activate the new virtualenv:
source env/bin/activate
-And install the required dependecies:
pip install -r requirements.txt
-Now you have to tell MINARD where to look for the propper settings:
export MINARD_SETTINGS=`pwd`/settings.conf
-And you can run the server. This will run the server on localhost:5000. You can forward the
port through your SSH session to view it in a browser on your laptop:
python runserver.py




