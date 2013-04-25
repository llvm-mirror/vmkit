#/bin/bash

. ./float.sh

# calc_count
# stdin holds one number per line
function calc_count()
{
	local count=0

	while read i ; do
		count=$(($count + 1))
	done

	echo -n $count
}

# calc_average
# stdin holds one number per line
function calc_average()
{
	local average=0
	local count=0

	while read i ; do
		count=$(($count + 1))
		average=$(float_eval $average + $i)
	done

	average=$(float_eval $average / $count)
	echo -n $average
}

# calc_variance $average
# stdin holds one number per line
function calc_variance()
{
	local average=$1
	local count=0
	local variance=0

	while read i ; do
		count=$(($count + 1))
		variance=$(float_eval $variance + '(' $i - $average ')^2' )
	done

	variance=$(float_eval $variance / $count)
	echo -n $variance
}

# calc_standard_deviation $average
# stdin holds one number per line
function calc_standard_deviation()
{
	local average=$1
	float_eval 'sqrt(' $(calc_variance $average) ')'
}

# rebase_time_then_format $base_time_in_seconds $timefmt $current_time_in_seconds
function rebase_time_then_format()
{
	local base_time=$1
	local timefmt=$2
	local current_time=$3

	local diff_sec=$(($current_time - $base_time))
	date "+$timefmt" -d "2000-01-01 00:00:00 CET + $diff_sec seconds"
}
