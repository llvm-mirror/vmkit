#!/bin/bash

enable=$1

if [ "$enable" -ne "1" ]; then exit 0; fi

vm_name=$2
bench_version=$3
jvm=$4
jar=$5
vmargs=$6
bench_name=$7

dacapo_dir=/home/koutheir/PhD/VMKit/vmkit2/tests/dacapo

suffix=_${bench_version}_${bench_name}_${vm_name}

scratch_dir=$(mktemp -d --suffix=$suffix --tmpdir=$dacapo_dir/tmp)
log_file=$(mktemp --suffix=${suffix}.log --tmpdir=$dacapo_dir/logs)

cd "$scratch_dir"
"$jvm" -jar "$jar" $vmargs "$bench_name" > $log_file 2>&1 || exit 1

cd ..
rm -rf "$scratch_dir"
exit 0
