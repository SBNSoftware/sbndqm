#!/bin/bash

STREAMS="10000"

if [ "$#" -eq "0" ]
then
    PW=""
else
    PW="-pw $1"
fi

python CreateConfig.py -q $STREAMS
python Fill.py $PW

for  ((j = 10 ; j < 11 ; j++))
do
    rm logs/*
    for ((i = 0 ; i < 50 ; i++))
    do
	python Archiver.py -pr $j $PW >> logs/times.log
    done
    echo "Number of processes: $j"
    python RunTimeAverages.py
done
