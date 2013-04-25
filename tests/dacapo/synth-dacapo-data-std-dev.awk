BEGIN {
	FS = ",";

	getline;
	print $0;

	getline;

	bench_suite		= $1;
	benchmark		= $2;
	vm				= $3;
	duration_ms		= $4;
	avg_duration_ms	= $5;
	min_duration_ms	= $6;
	max_duration_ms	= $7;
	std_dev_duration_ms = (duration_ms - avg_duration_ms)^2;
	count			= 1;

	print $0;
}
/^$/ { /* Ignore empty lines */ }
{
	if ($1 == bench_suite && $2 == benchmark && $3 == vm) {
		std_dev_duration_ms += ($4 - $5)^2;
		count++;
	} else {
		std_dev_duration_ms = sqrt(std_dev_duration_ms / count);
		printf("%s" FS "%s" FS "%s" FS "-" FS "%.2f" FS "%.2f" FS "%.2f" FS "%.2f" RS,
			bench_suite, benchmark, vm,
			avg_duration_ms, min_duration_ms, max_duration_ms, std_dev_duration_ms);

		bench_suite		= $1;
		benchmark		= $2;
		vm				= $3;
		duration_ms		= $4;
		avg_duration_ms	= $5;
		min_duration_ms	= $6;
		max_duration_ms	= $7;
		std_dev_duration_ms = (duration_ms - avg_duration_ms)^2;
		count			= 1;
	}

	print $0;
}
END {
	std_dev_duration_ms = sqrt(std_dev_duration_ms / count);
	printf("%s" FS "%s" FS "%s" FS "-" FS "%.2f" FS "%.2f" FS "%.2f" FS "%.2f" RS,
		bench_suite, benchmark, vm,
		avg_duration_ms, min_duration_ms, max_duration_ms, std_dev_duration_ms);
}
