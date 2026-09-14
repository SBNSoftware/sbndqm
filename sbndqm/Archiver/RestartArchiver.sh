#!/bin/bash

# this script should only be run as user `icarus`

if [[ "$(whoami)" != "icarus" ]]; then
    echo "This script must be run as user icarus, now as $(whoami)"
    exit 1
fi

echo "Remaining OnMon Archiver.py processes:"
ps aux | grep '[p]ython Archiver.py'

echo "Killing remaining OnMon Archiver.py processes..."

toKillProc=$(ps aux | grep '[p]ython Archiver.py' | awk '{print $2}')
if [[ $toKillProc ]] ; then
    kill -9 $toKillProc
else
    echo "No OnMon Archiver.py processes remaning..."
fi


echo "Restarting OnMon Archiver."
cd /home/nfs/icarus/Archiver/sbndqm/sbndqm/Archiver
source env/bin/activate
python Archiver.py -pr 1 &

echo "OnMon Archiver restarted."
