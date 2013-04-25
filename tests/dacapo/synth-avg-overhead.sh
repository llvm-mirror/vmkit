#!/bin/bash

# Generate overhead between a reference engine (e.g. reference VM stats) and another engine (e.g. feature-enabled VM stats).
# Gets in input two synthesized and reduced stats data.

. ./float.sh
float_scale=4

ref_stats=$1
new_stats=$2

ref_header=$(head -1 "$ref_stats")

echo "$(head -1 "$ref_stats" | cut -d ',' -f 1-3),avg_overhead"

for i in $(grep -v '^#' "$ref_stats") ; do
	key=$(echo "$i" | cut -d ',' -f 1-3)
	ref_val=$(echo "$i" | cut -d ',' -f 4)
	new_val=$(grep "^$key," "$new_stats" | cut -d ',' -f 4)
	overhead=$(float_eval "100.0 * ( $new_val / $ref_val - 1.0 )" )
	printf "%s,%.2f\n" "$key" "$overhead"
done
