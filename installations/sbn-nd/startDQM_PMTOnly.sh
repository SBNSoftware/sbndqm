#!/bin/bash

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
  source /daq/software/products/setup
  source $MRB_TOP/localProducts_sbndqm_v1_02_00_e20_prof/setup
  setup mrb
  mrbsetenv
  mrbslp
  cd $MRB_SOURCE/sbndqm
  python sbndqm/DAQConsumer/daq_consumer.py -f installations/sbn-nd/SBND_OnlineMonitor_PMTOnly.fcl -l /daq/log/DAQConsumer/ &
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

