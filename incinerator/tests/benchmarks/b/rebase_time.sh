#!/bin/bash

input=$1
headline=$(head -n 1 "$input")
base_time=$(head -n 2 "$input"|tail -n 1|cut -d ',' -f 2)
timefmt=%M:%S

echo $headline

oldIFS=$IFS
IFS='
'
for entry in $(cat "$input"|grep '^[0-9]'); do
	pid=$(echo $entry|cut -d ',' -f 1)
	time=$(echo $entry|cut -d ',' -f 2)
	data=$(echo $entry|cut -d ',' -f '3-')
	
	sec=$(($time - $base_time))
	hourminsec=$(date "+$timefmt" -d "2000-01-01 00:00:00 CET + $sec seconds")
	echo "$pid,$hourminsec,$data"
done
IFS=oldIFS
