#!/bin/bash

echo "Compiling the project"
make project




config1="-c 10000000 --cacheline-size 32 --l1-lines 64 --l2-lines 128 --l1-latency 1 --l2-latency 2 --memory-latency 3"

rm output.txt
touch output.txt

echo "Running with parameters: $config1"
./project $config --tf=tracefile b.csv


echo "All simulations completed."
