#!/bin/bash +x

if [ $# -ne 5 ]; then
  echo " "
  echo " Usage: "
  echo "     createMonitor.sh <group> <metric> <monitor type: 0...4> <channel ID 1> <channel ID n>"
  echo " "
  echo " Where <postgres table name> will be constructed:"
  echo "         <GROUP>_<METRIC>_<MEAN|MODE|RMS|MIN|MAX|LAST>_MONITOR"
  echo "   e.g.  TPC_RMS_MEAN_MONITOR"
  echo " "
  exit
fi

#
# Modify the following psql parameters to suit your own datbase
#
psql -U runcon_admin -h sbnd-db -p 5434 -d sbnd_online_prd \
   --set=GROUP_NAME=\'$1\' --set=METRIC=\'$2\' --set=MONITOR_TYPE=\'$3\' \
   --set=CH1=$4 --set=CHN=$5 -c "\i channelMonitor.sql"

