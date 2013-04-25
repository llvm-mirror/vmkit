#/bin/bash

# program & ./mon-process.sh $!

. ./statistics.sh

input=$1
duration=$2

if [ 0 -ne "$input" ] 2>/dev/null ; then
	# Input is a PID
	input_type='-p'
else
	# Input is a command name
	input_type='-C'
fi

# Collect statistics data
rawdata="$(sudo nice -n -10 pidstat "$input_type" "$input" -hru 1 $duration)"

rawdata="$(echo "$rawdata" | grep '^[# 0-9]' | sed 's/^#//g' | sed 's/^ \+//g' | tr -s ' ' ',')"
head="$(echo "$rawdata" | head -1)"
data="$(echo "$rawdata" | grep -v '^[^0-9]')"

# Rebase time
base_time=$(echo "$data" | head -1 | cut -d ',' -f 1)
timefmt=%M:%S

echo "$head"
for i in $data ; do
	old_time=$(echo "$i" | cut -d ',' -f 1)
	new_time=$(rebase_time_then_format $base_time $timefmt $old_time)

	remaining_data=$(echo "$i" | cut -d ',' -f 2-)
	echo "$new_time,$remaining_data"
done
