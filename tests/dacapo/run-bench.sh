#!/bin/bash

# Run a Dacapo benchmark, logging its results

enable=$1

if [ "$enable" -ne "1" ]; then exit 0; fi

vm_name=$2
bench_version=$3
jvm=$4
jar=$5
vmargs=$6
bench_name=$7

#dacapo_dir=/home/koutheir/PhD/VMKit/vmkit/tests/dacapo
dacapo_dir=$HOME/dacapo_benchmarks
tmp_dir=$dacapo_dir/tmp
log_dir=$dacapo_dir/logs

suffix=_${bench_version}_${bench_name}_${vm_name}

mkdir -p "$tmp_dir" "$log_dir" 2>/dev/null
scratch_dir=$(mktemp -d --suffix=$suffix --tmpdir=$tmp_dir)
log_file=$(mktemp --suffix=${suffix}.log --tmpdir=$log_dir)

curdir=$(pwd)
cd "$scratch_dir"
echo "$jvm" -jar "$jar" $vmargs "$bench_name"
"$jvm" -jar "$jar" $vmargs "$bench_name"
# "$jvm" -jar "$jar" $vmargs "$bench_name" > $log_file 2>&1
r=$?

cd "$curdir"
rm -rf "$scratch_dir"
exit $r
