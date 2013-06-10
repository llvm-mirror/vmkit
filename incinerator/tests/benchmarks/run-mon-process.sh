#!/bin/bash

$* &
./mon-process.sh $!
