#!/bin/bash

# Augment input data with statistical data about them: min, max, average, standard deviation

# Augment statistical data with min, max and average
data=$(awk -f synth-dacapo-data-min-max-avg.awk)
header=$(echo "$data" | head -1)

# Integrate augmented information into given input
data=$(echo "$data" | tail -n $(( $(echo "$data" | wc -l) - 1)) )
added_data=$(echo "$data" | grep -v ',-,-,-,-')

augmented_data=$header

for i in $added_data ; do
	key=$(echo "$i" | cut -d ',' -f 1-3)
	added_value=$(echo "$i" | cut -d ',' -f 5-)
	augmented_data+=$'\n'
	augmented_data+=$(echo "$data" | grep "^$key,[^-]" | sed "s/-,-,-,-/$added_value/g")
done

# Augment statistical data with standard deviation
data=$(echo "$augmented_data" | awk -f synth-dacapo-data-std-dev.awk)

# Integrate augmented information into given input
data=$(echo "$data" | tail -n $(( $(echo "$data" | wc -l) - 1)) )
added_data=$(echo "$data" | grep -v ',-$')

augmented_data=$header

for i in $added_data ; do
	key=$(echo "$i" | cut -d ',' -f 1-3)
	added_value=$(echo "$i" | cut -d ',' -f 8)

	augmented_data+=$'\n'
	augmented_data+=$(echo "$data" | grep "^$key,[^-]" | sed "s/,-/,$added_value/g")
done

echo "$augmented_data"
