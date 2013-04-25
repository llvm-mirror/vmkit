#!/bin/bash

. ./float.sh

# Converts size from some size unit to bytes unit.
function get_size_in_bytes()
{
	local size=$1
	local number=$(echo "$size" | cut -d ' ' -f 1)
	local unit=$(echo "$size" | cut -d ' ' -f 2)
	local factor=1

	case $unit in
	B ) factor=1 ;;
	kB ) factor=1024 ;;
	mB ) factor=1048576 ;;
	gB ) factor=1073741824 ;;
	tB ) factor=1099511627776 ;;
	esac
	
	float_eval "$number * $factor"
}
