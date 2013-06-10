#!/bin/bash

# Compute average, variance, and standard deviation given a list of numbers.

. ./statistics.sh

data="$(tr -s ' ' '\n' | tr -s '\n')"

average=$(echo "$data" | calc_average)
count=$(echo "$data" | calc_count $average)
variance=$(echo "$data" | calc_variance $average)
std_dev=$(echo "$data" | calc_standard_deviation $average)

echo "count=$count"
echo "average=$average"
echo "variance=$variance"
echo "standard deviation=$std_dev"
