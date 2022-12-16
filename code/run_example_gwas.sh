#!/bin/sh
for (( i = 0; i <= 2; i++ ))
do
    echo "Running PID=$i"
    CMD="bin/GwasClient $i ../par/test.par.$i.txt"
    
    echo $CMD
    if [ $i = 2 ]; then
        eval $CMD
    else
        eval $CMD &
    fi
    sleep 1
done

