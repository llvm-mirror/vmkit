#!/bin/bash

# Removes elementary data and only leave satistical data (min, max, average, standard deviation)

log_dir=$1

./synth-logs.sh "$log_dir" | sed 's/^\([^,]*,[^,]*,[^,]*\),[^,]*/\1/g' | uniq
