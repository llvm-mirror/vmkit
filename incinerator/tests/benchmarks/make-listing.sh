#!/bin/bash

awk -e '{printf("%.2d %s\n", NR, $0);}'
