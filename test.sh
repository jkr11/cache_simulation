#!/bin/bash

echo "Cleaning previous builds..."
make clean

echo "Compiling the project"
make project


echo "Running project"

config1 = " -c 100 --cacheline-size 8 --l1-lines 128 --l2-lines 256 --l1-latency 1 --l2-latency 3 --memory-latency 10 --tf=tracefile.csv example.csv"

./project $config1 --tf=tracefile f.csv

echo "Ran with parameters: $config1"
