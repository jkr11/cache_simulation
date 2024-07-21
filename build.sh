#!/bin/bash
echo "Testing for Artemis"
echo "with ./project -c 10000000 --cacheline-size 16 --l1-lines 32 --l2-lines 128 --l1-latency 4 --l2-latency 16 --memory-latency 400 --tf=tracefile examples/c.csv"

./project -c 10000000 --cacheline-size 16 --l1-lines 32 --l2-lines 128 --l1-latency 4 --l2-latency 16 --memory-latency 400 --tf=tracefile examples/c.csv

echo "Finished?"