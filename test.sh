#!/bin/bash

echo "Cleaning previous builds..."
make clean

echo "Compiling the project"
make project

echo "Generating cache accesses"
python3 input.py 16

config = "-c 10000000 --cacheline-size 16 --l1-lines 128 --l2-lines 256 --l1-latency 1 --l2-latency 3 --memory-latency 10 --tf=tracefile"
echo "Running with "
./project $config cache_accesses.csv
