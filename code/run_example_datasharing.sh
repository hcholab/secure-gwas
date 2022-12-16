#!/bin/sh
for (( i = 0; i <= 2; i++ ))
do
    echo "Running PID=$i"
    CMD="bin/DataSharingClient $i ../par/test.par.$i.txt"
    
    if [ $i -ne 0 ]; then
        CMD="$CMD ../test_data$i/"
    fi
    echo $CMD
    if [ $i = 2 ]; then
        eval $CMD
    else
        eval $CMD &
    fi
    sleep 1
done

