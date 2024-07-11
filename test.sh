#!/bin/bash

echo "Compiling the project"
make project

echo "Running multiple times"


config1="-c 1000000 --cacheline-size 32 --l1-lines 64 --l2-lines 128 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config2="-c 1000000 --cacheline-size 32 --l1-lines 64 --l2-lines 128 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config3="-c 1000000 --cacheline-size 32 --l1-lines 64 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config4="-c 1000000 --cacheline-size 32 --l1-lines 64 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config5="-c 1000000 --cacheline-size 64 --l1-lines 128 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config6="-c 1000000 --cacheline-size 64 --l1-lines 128 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config7="-c 2000000 --cacheline-size 32 --l1-lines 64 --l2-lines 128 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config8="-c 2000000 --cacheline-size 32 --l1-lines 64 --l2-lines 128 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config9="-c 2000000 --cacheline-size 32 --l1-lines 64 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config10="-c 2000000 --cacheline-size 32 --l1-lines 64 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config11="-c 2000000 --cacheline-size 64 --l1-lines 128 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config12="-c 2000000 --cacheline-size 64 --l1-lines 128 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config13="-c 3000000 --cacheline-size 32 --l1-lines 64 --l2-lines 128 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config14="-c 3000000 --cacheline-size 32 --l1-lines 64 --l2-lines 128 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config15="-c 3000000 --cacheline-size 32 --l1-lines 64 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config16="-c 3000000 --cacheline-size 32 --l1-lines 64 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config17="-c 3000000 --cacheline-size 64 --l1-lines 128 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"
config18="-c 3000000 --cacheline-size 64 --l1-lines 128 --l2-lines 256 --l1-latency 5 --l2-latency 15 --memory-latency 30"

rm output.txt
touch output.txt
touch tracefile.vcd
for config in "$config1" "$config2" "$config3" "$config4" "$config5" "$config6" "$config7" "$config8" "$config9" "$config10" "$config11" "$config12" "$config13" "$config14" "$config15" "$config16" "$config17" "$config18"; do
    rm tracefile.vcd
    # TODO: update this with examples/
    echo "Running with parameters: $config"
    ./project $config --tf=tracefile test/cache_simulation_data.csv
    
done

echo "All simulations completed."
