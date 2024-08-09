#!/bin/bash

echo "Remaining OnMon Archiver.py processes:"
ps aux | grep '[p]ython Archiver.py'

echo "Killing remaining OnMon Archiver.py processes..."
kill -9 $(ps aux | grep '[p]ython Archiver.py' | awk '{print $2}')

echo "Restarting OnMon Archiver."
cd /home/nfs/icarus/Archiver/sbndqm/sbndqm/Archiver
source env/bin/activate
python Archiver.py -pr 1 &

echo "OnMon Archiver restarted."
