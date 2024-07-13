#!/bin/bash

echo "Cleaning previous builds..."
make clean

echo "Compiling the project"
make project

echo "Generating cache accesses"



./project -c 100000 --cacheline-size 64 --l1-lines 32 --l2-lines 128 --l1-latency 4 --l2-latency 16 --memory-latency 400 --tf=tracefile cache_accesses_32.csv
