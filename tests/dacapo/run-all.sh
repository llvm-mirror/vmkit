#!/bin/bash 

# Run all Dacapo tests muliples times, logging their results
# ./run-all.sh 40 12 2006-10-MR2 'sleep $(($RANDOM / 7276))s ; echo Hello'

times=$1
# jn=$2
dir=$2
cmd=$3

cd "$dir"
$cmd

exit

seq $times | parallel -n0 -j "$jn" -- $cmd
