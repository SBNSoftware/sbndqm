#!/bin/bash

# 18 March 2020 -- copied from commited version...
# Change name of terminal, suggested by Antoni
printf '\033]2;Online Monitoring\a'

#defaults
my_env="sbn-fd"
my_devdir=$1
my_redishost=icarus-db01.fnal.gov
artdaq_version=v3_12_05
artdaq_quals=e20:s120a:prof
sbncode_quals=e20:prof
my_swdir=/daq/software

my_swdir=/daq/software
my_daqarea=${HOME}/DQM_DevAreas

fcl_file=$my_daqarea/$my_devdir/srcs/sbndqm/installations/sbn-fd/$2
my_dispatcherfcl=" -f $fcl_file "

unset PRODUCTS

printf "Environment:\n"
printf "\tenv=$my_env\n"
printf "\tdevdir=$my_devdir\n"
printf "\tdispatcherfcl=$fcl_file\n"
printf "\tredishost=$my_redishost\n\n\n"

source $my_swdir/products/setup

cd $my_daqarea/$my_devdir
setup mrb
source $(ls -f $(pwd)/localProducts*/setup |head -1) > /dev/null
mrbslp
echo "sbndqm setup"


setup icaruscode $SBNCODE_VERSION -q $sbncode_quals
echo "icaruscode setup"

ups active | grep -E "icaruscode"

unsetup artdaq_core
unsetup TRACE

setup artdaq $artdaq_version -q $artdaq_quals

echo "artdaq setup"

ups active |grep -E "(sbndqm)"

my_pythonbin=$(dirname $(which python))
my_pythonpath=$PYTHONPATH
unset PYTHONPATH

if [[ ! -d $my_daqarea/$my_devdir/python_virtualenv ]]; then
  $my_pythonbin/pip install virtualenv
	$my_pythonbin/virtualenv -p $my_pythonbin/python --system-site-packages python_virtualenv
  source $my_daqarea/$my_devdir/python_virtualenv/bin/activate
  pip install --upgrade pip
  pip install -r $SBNDQM_DIR/tools/AliveMonitor/requirements.txt
	pip list -v
else
  source $my_daqarea/$my_devdir/python_virtualenv/bin/activate
fi

export FHICL_FILE_PATH=$my_daqarea/$my_devdir:$FHICL_FILE_PATH
export PYTHONPATH=$(echo ${my_pythonpath} | awk -v RS=: -v ORS=: '/site-packages/ {next} {print}'| sed 's/:*$//' )

#cleanup
killall -9 lar
ipcrm -M 0x40471454

python $SBNDQM_FQ_DIR/../tools/AliveMonitor/alive_monitor.py -s "$my_redishost" -k DAQConsumer \
-c "python $SBNDQM_FQ_DIR/../tools/DAQConsumer/daq_consumer.py $my_dispatcherfcl -l /daq/log/DAQConsumerOM/ -lo"
