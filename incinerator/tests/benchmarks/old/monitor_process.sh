#/bin/bash

# ps -ww -o etime:1,pid:1,time:1,%cpu,%mem,rss -C "$1" # |tr -s ' ' ','

float_scale=2
page_size=$(getconf PAGESIZE)
clock_tick=$(getconf CLK_TCK)

# Evaluate then output an equation into a float number with $float_scale precision.
function float_eval()
{
	local stat=0
	local result=0.0
	
	if [[ $# -gt 0 ]]; then
		result=$(echo "scale=$float_scale; $*"|bc -q 2>/dev/null)
		stat=$?
		
		if [[ "$stat" -eq 0 && -z "$result" ]]; then stat=1; fi
	fi
	
	echo -n $result
	return $stat
}

# Evaluate an equation into a condition.
function float_cond()
{
	local cond=0
	
	if [[ $# -gt 0 ]]; then
		cond=$(echo "$*"|bc -q 2>/dev/null)
		
		if [[ -z "$cond" ]]; then cond=0; fi
		if [[ "$cond" != 0 && "$cond" != 1 ]]; then cond=0; fi
	fi
	
	local stat=$(($cond == 0))
	return $stat
}

# Outputs a PID given a process name.
# If multiple processes of the same program exist, only one PID will be output.
function get_pid_from_name()
{
	local process_name=$1

	ps --no-headers -ww -o pid:1 -C "$process_name"|head -n 1
	return $?
}

# Returns the memory data usage in bytes for a PID
function get_memory_data_usage()
{
	local pid=$1
	if [ -z "$pid" ]; then return 1; fi;
	
	local memory_data_pages=$(cat /proc/$pid/statm 2>/dev/null|cut -d ' ' -f 6)
	if [ -z "$memory_data_pages" ]; then return 1; fi;

	float_eval "$memory_data_pages * $page_size"
}

# Converts size from some size unit to bytes unit.
function get_size_in_bytes()
{
	local size=$1
	local number=$(echo "$size"|cut -d ' ' -f 1)
	local unit=$(echo "$size"|cut -d ' ' -f 2)
	local factor=1

	case $unit in
	B ) factor=1 ;;
	kB ) factor=1024 ;;
	mB ) factor=1048576 ;;
	gB ) factor=1073741824 ;;
	esac
	
	float_eval "$number * $factor"
}

# Returns writable memory usage, given a PID.
function get_writable_memory_usage()
{
	local pid=$1
	if [ -z "$pid" ]; then return 1; fi;

	local sum=0
	local oldIFS=$IFS
IFS='
'
	for entry in $(cat /proc/$pid/smaps|tr -s ' '|grep '^Private_Dirty: [^0]'|cut -d ' ' -f '2,3'); do
		sum=$(float_eval "$sum + $(get_size_in_bytes $entry)")
	done
	IFS=oldIFS

	echo $sum
}

# Returns the CPU time (user+kernel) in seconds, given a PID.
function get_cpu_time()
{
	local pid=$1
	if [ -z "$pid" ]; then return 1; fi;

	local user_time=$(cat /proc/$pid/stat 2>/dev/null|cut -d ' ' -f 14)
	if [ -z "$user_time" ]; then return 1; fi;

	local kernel_time=$(cat /proc/$pid/stat 2>/dev/null|cut -d ' ' -f 15)
	if [ -z "$kernel_time" ]; then return 1; fi;

	float_eval "($user_time + $kernel_time) / $clock_tick"
}

# Outputs the CPU average usage, given a PID.
function get_cpu_avg_usage()
{
	local pid=$1
	local avg_period=$2
	local _cpu_avg_usage=$3
	local _last_cpu_time=$4
	local last_cpu_time

	eval last_cpu_time='$'$_last_cpu_time''

	if [ -z "$pid" ]; then return 1; fi;

	if [ -z "$last_cpu_time" ]; then
		last_cpu_time=$(get_cpu_time $pid)
		if [ -z "$last_cpu_time" ]; then return 1; fi;
	fi;

	local cpu_time=$(get_cpu_time $pid)
	if [ -z "$cpu_time" ]; then return 1; fi;

	local cpu_avg_usage=$(float_eval "100 * ( $cpu_time - $last_cpu_time ) / $avg_period")
#	local cpu_avg_usage=$(float_eval "$cpu_time - $last_cpu_time"),$(float_eval "100 * ( $cpu_time - $last_cpu_time ) / $avg_period")

	eval $_cpu_avg_usage="'$cpu_avg_usage'"
	eval $_last_cpu_time="'$cpu_time'"
	return 0
}

period=
the_date=
the_last_date=$(date '+%s')
the_last_cpu_time=
the_cpu_avg_usage=

sleep 1

echo "PID,DATE,MEM_DATA,CPU_USAGE"
#echo "DATE,PID,MEM_DATA,CPU_TIME,CPU_TIME_DIFF,CPU_USAGE"

while true
do
	pid=$(get_pid_from_name "$1")

	if [ ! -z "$pid" ]; then
		the_date=$(date '+%s')
		period=$(($the_date - $the_last_date))
		the_last_date=$the_date

		if [ "$period" -gt "0" ]; then
			get_cpu_avg_usage $pid $period the_cpu_avg_usage the_last_cpu_time

#			echo "$(date '+%F %T'),$pid,$(get_writable_memory_usage $pid),$(get_cpu_time $pid),$the_cpu_avg_usage"
#			echo "$(date -d "@$the_date" '+%F %T'),$pid,$(get_writable_memory_usage $pid),$the_cpu_avg_usage"
			echo "$pid,$the_date,$(get_writable_memory_usage $pid),$the_cpu_avg_usage"
		fi;
	fi;
done
