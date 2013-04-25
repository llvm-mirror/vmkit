#!/bin/bash 

# ./run-all.sh 40 12 2006-10-MR2 'sleep $(($RANDOM / 7276))s ; echo Hello'

times=$1
jn=$2
dir=$3
cmd=$4

cd "$dir"
seq $times | parallel -n0 -j "$jn" -- $cmd
