#!/bin/bash

# 18 March 2020 -- copied from commited version...
# Change name of terminal, suggested by Antoni
printf '\033]2;Online Monitoring\a'

#defaults
my_env="sbn-fd"
my_devdir=$1
my_redishost=icarus-db02.fnal.gov
my_swdir=/daq/software

my_daqarea=${HOME}/DQM_SPACK_DevAreas

fcl_file=$my_daqarea/$my_devdir/sbndqm/installations/sbn-fd/$2
my_dispatcherfcl=" -f $fcl_file "

printf "Environment:\n"
printf "\tenv=$my_env\n"
printf "\tdevdir=$my_devdir\n"
printf "\tdispatcherfcl=$fcl_file\n"
printf "\tredishost=$my_redishost\n\n"

echo "Activating Spack environment..."
source /daq/software/spack_packages/spack/v1.0.1.sbnd/setup-env.sh 
export SPACK_DISABLE_LOCAL_CONFIG=true
cd $my_daqarea/$my_devdir
spack env activate --prompt --dir .

echo "$(spack find --format '{name}@{version} {variants.s} {variants.cxxstd} /{hash:7}' sbndqm-suite)"
echo "$(spack find --format '{name}@{version} {variants.cxxstd} /{hash:7}' sbndqm)"
echo "$(spack find --format '{name}@{version} {variants.cxxstd} /{hash:7}' sbndaq-online)"
echo "$(spack find --format '{name}@{version} {variants.cxxstd} /{hash:7}' icaruscode)"

echo "Loaded Spack enviroment!"

if [[ ! -d $my_daqarea/$my_devdir/python_virtualenv ]]; then
	python3 -m venv $my_daqarea/$my_devdir/python_virtualenv
	source $my_daqarea/$my_devdir/python_virtualenv/bin/activate
	python3 -m pip install --upgrade pip
	python3 -m pip install -r $my_daqarea/$my_devdir/sbndqm/sbndqm/AliveMonitor/requirements.txt
	python3 -m pip list -v
else
	source $my_daqarea/$my_devdir/python_virtualenv/bin/activate
fi

export PYTHONPATH="$(spack location -i py-fhicl-py)/lib:$PYTHONPATH"

FCL_DIR="$(dirname "$fcl_file")"
export FHICL_FILE_PATH="$FCL_DIR:$my_daqarea/$my_devdir:$FHICL_FILE_PATH"

#cleanup
killall -9 lar
ipcrm -M 0x40471454

python $my_daqarea/$my_devdir/sbndqm/sbndqm/AliveMonitor/alive_monitor.py -s "$my_redishost" -k DAQConsumer \
-c "python $my_daqarea/$my_devdir/sbndqm/sbndqm/DAQConsumer/daq_consumer.py $my_dispatcherfcl -l /daq/log/DAQConsumerOM/ -lo"
