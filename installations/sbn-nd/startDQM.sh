#!/bin/bash

MRBDIR=/home/nfs/sbnddqm/DQM_DevAreas/MJ_27Mar2024/

function printhelp() {
  echo "Starts the online monitoring. Can be run in the foreground or background."
  echo "To stop while running in the foreground, type Ctrl-C to send SIGINT."
  echo "To stop while tunning in the background, run:"
  echo "kill -INT <pid>"
  echo "where <pid> is the process ID of either the startDQM process or the child"
  echo "python process."
  exit
}

function cleanup() {
  trap forceexit SIGINT
  do_cleanup
}

function do_cleanup() {
  
  echo "Cleaning up and Exiting"
  if [ ! -z ${DAQConsumer+x} ]
  then
    sleep 1
    kill -INT $DAQConsumer 2> /dev/null
    wait $DAQCOnsumer
  fi
  if [ ! -z ${PIDFILE+x} ]
  then
    rm -f $PIDFILE
  fi
  exit
}

function forceexit() {
  echo "Exiting without cleaning up"
  rm -f $PIDFILE
  exit
}

function fail() {
  echo "Unable to obtain lock file -- DQM is already running. If you believe this is mistaken, try removing the lock file: rm -f $PIDFILE"
  exit 1
}

function main() {
  trap cleanup SIGINT
  cd $MRBDIR
  source /daq/software/products/setup
  setup mrb
  source localProducts_sbndqm_v1_03_00_e26_prof/setup
  mrbsetenv
  unsetup artdaq_core
  setup artdaq v3_12_07 -f Linux64bit+3.10-2.17 -q e26:prof:s120a -z /daq/software/products
  cd $MRB_SOURCE/sbndqm/installations/sbn-nd
  python ../../sbndqm/DAQConsumer/daq_consumer.py -f /home/nfs/sbnddqm/DQM_DevAreas/MJ_27Mar2024/srcs/sbndqm/installations/sbn-nd/online_tpc_analysis-skipevt0.fcl -l /daq/log/DAQConsumer/ &
  DAQConsumer=$! 
  echo "Online Monitoring Started"
  wait $DAQConsumer
  echo "Online Monitoring Exited"
}

if  [ "$1" = "-h" ]
then
  printhelp
fi


PIDFILE=/tmp/dqm.pid
(
  flock -n 9 || fail
  main
) 9>$PIDFILE

