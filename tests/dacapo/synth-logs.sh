#!/bin/bash 

# Collect results from benchmarks logs, then synthesize them, producing statistical data about them.

log_dir=$1
cur_dir=$(pwd)

cd "$log_dir"
data=$(echo '#bench_suite,benchmark,vm,duration_ms')
data+=$'\n'
data+=$(grep '^===== DaCapo [A-Za-z]\+ PASSED' tmp.* | sed 's/tmp[^_]\+_//g' | sed 's/ msec =====//g' | sed 's/\.log:===== DaCapo [a-z]\+ PASSED in /_/g' | tr '_' ',' | sort)

cd "$cur_dir"
echo "$data" | ./augment-dacapo-data.sh
