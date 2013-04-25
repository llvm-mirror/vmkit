#!/bin/bash 

dacapo_dir=/home/koutheir/PhD/VMKit/vmkit2/tests/dacapo
log_collect_file=$dacapo_dir/logs/collected_passed.log
log_synth_file=$dacapo_dir/logs/synth.log

synth_awk=$(cat <<EOF
BEGIN {
	FS = ",";
	getline;

	bench_version = \$1;
	bench_name = \$2;
	vm_name = \$3;
	val = \$4;
	nval = 1;
}
/^\$/ {}
{
	if (\$1 == bench_version && \$2 == bench_name && \$3 == vm_name) {
		val += \$4;
		nval++;
	} else {
		val /= nval;
		print bench_version FS bench_name FS vm_name FS int(val);

		bench_version = \$1;
		bench_name = \$2;
		vm_name = \$3;
		val = \$4;
		nval = 1;
	}
}
END {
	val /= nval;
	print bench_version FS bench_name FS vm_name FS val;
}
EOF
)

cd "$dacapo_dir/logs"
grep '^===== DaCapo [A-Za-z]\+ PASSED' tmp.* | sed 's/tmp[^_]\+_//g' | sed 's/ msec =====//g' | sed 's/\.log:===== DaCapo [a-z]\+ PASSED in /_/g' | tr '_' ',' | sort > $log_collect_file
awk -e "$synth_awk" "$log_collect_file" > $log_synth_file
