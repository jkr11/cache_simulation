#!/bin/bash

echo "Cleaning previous builds..."
make clean

echo "Compiling the project"
make all


echo "Running project"

./project -c 100 --cacheline-size 64 --l1-lines 128 --l2-lines 256 --l1-latency 1 --l2-latency 3 --memory-latency 10 --tf=tracefile.csv example.csv