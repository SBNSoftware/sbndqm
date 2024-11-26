#!/bin/bash

#defaults
my_quals="e19:prof:s94:py2"
my_version="v0_06_00"
my_env="sbn-fd"
my_gitbranch="develop"
#my_devdir=$(date +"DAQ_%d%b%C")
my_devdir=DAQ_02Jun20
my_projname=sbndqm
my_new=False
my_redishost=icarus-db02.fnal.gov
my_dispatcherfcl=" -f dispatcher_sbndqm_v3.fcl "

while [[ "$#" -gt 0 ]]; do case $1 in
  -e|--env) my_env="$2"; shift;;
  -d|--devdir) my_devdir="$2"; shift;;
  -v|--version) my_version="$2"; shift;;
  -q|--quals) my_quals="$2"; ishift;;
	-f|--dispatcherfcl) my_dispatcherfcl="$2"; shift;;
	-r|--redishost) my_redishost="$2"; shift;;
	-b|--gitbranch) my_gitbranch="$2"; shift;;
	-p|--projname) my_projname="$2"; shift;;
  -n|--new) my_new=True;;
  *) echo "Unknown parameter passed: $1"; exit 1;;
esac; shift; done

my_swdir=/daq/software
my_daqarea=${HOME}/DQM_DevAreas
unset PRODUCTS

printf "Options:\n"
printf "\tnew=$my_new\n"
printf "\tenv=$my_env\n"
printf "\tquals=$my_quals\n"
printf "\tversion=$my_version\n"
printf "\tdevdir=$my_devdir\n"
printf "\tdispatcherfcl=$my_dispatcherfcl\n"
printf "\tredishost=$my_redishost\n\n\n"
printf "Usage:\n"
printf " $(basename "$0") -e $my_env -v $my_version -q $my_quals -d $my_devdir -f $my_dispatcherfcl \n\n"

source $my_swdir/products/setup
[[ -f $my_swdir/products_dev/setup ]] && source $my_swdir/products_dev/setup

if [[ $my_new ]]; then
  if [[ ! -d $my_daqarea/$my_devdir ]]; then
    mkdir -p $my_daqarea/$my_devdir
    cd $my_daqarea/$my_devdir
    unsetup_all
    setup mrb
    export MRB_PROJECT=$my_projname
    mrb newDev -v "$my_version" -q "$my_quals"
    source $(ls -f $(pwd)/localProducts*$my_version*/setup |head -1)
    cd $MRB_SOURCE
    mrb g -d sbndqm ssh://p-sbndqm@cdcvs.fnal.gov/cvs/projects/sbndqm
    cd sbndqm
    git checkout "$my_gitbranch"
    cd ..
    mrbsetenv
    mrb i -j8
    unsetup_all
  fi
fi

cd $my_daqarea/$my_devdir
setup mrb
source $(ls -f $(pwd)/localProducts*$my_version*/setup |head -1) > /dev/null
setup sbndqm $my_version -q $my_quals

ups active |grep -E "(sbndqm)"

#printenv |grep DQM

my_pythonbin=$(dirname $(which python))
my_pythonpath=$PYTHONPATH
unset PYTHONPATH

if [[ ! -d $my_daqarea/$my_devdir/python_virtualenv ]]; then
  $my_pythonbin/pip install virtualenv
	$my_pythonbin/virtualenv -p $my_pythonbin/python --system-site-packages python_virtualenv
  source $my_daqarea/$my_devdir/python_virtualenv/bin/activate
  pip install --upgrade pip
  pip install -r $SBNDQM_FQ_DIR/tools/AliveMonitor/requirements.txt
	pip list -v
else
  source $my_daqarea/$my_devdir/python_virtualenv/bin/activate
fi

export FHICL_FILE_PATH=$my_daqarea/$my_devdir:$FHICL_FILE_PATH
export PYTHONPATH=$(echo ${my_pythonpath} | awk -v RS=: -v ORS=: '/site-packages/ {next} {print}'| sed 's/:*$//' )

python $SBNDQM_FQ_DIR/tools/AliveMonitor/alive_monitor.py -s "$my_redishost" -k DAQConsumer \
 -c "python $SBNDQM_FQ_DIR/tools/DAQConsumer/daq_consumer.py  $my_dispatcherfcl"


