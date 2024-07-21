#!/bin/bash

MANY_CYCLES=2000000000

LIGHT_GREEN='\033[1;32m'
LIGHT_RED='\033[1;31m'
LIGHT_BLUE='\033[1;34m'
MAGENTA='\033[1;35m'
NO_COLOR='\033[0m'

echo -en "${LIGHT_BLUE}Cycles: ${NO_COLOR}"
read CYCLES

echo -en "${LIGHT_BLUE}Cacheline-size: ${NO_COLOR}"
read LINESIZE

echo -en "${LIGHT_BLUE}L1-lines: ${NO_COLOR}"
read L1LINES

echo -en "${LIGHT_BLUE}L2-lines: ${NO_COLOR}"
read L2LINES

echo -en "${LIGHT_BLUE}L1-latency: ${NO_COLOR}"
read L1LAT

echo -en "${LIGHT_BLUE}L2-latency: ${NO_COLOR}"
read L2LAT

echo -en "${LIGHT_BLUE}Memory-latency: ${NO_COLOR}"
read MEMLAT


if [ ! -d ./tmp/ ]; then
  mkdir ./tmp/
fi

#Compiling the tester (if not already)
if [ ! -f "./test_data_validity/tester" ]; then
	echo "Compiling the tester ..."
  	TESTER_DIR="./test_data_validity"
  	make -C "$TESTER_DIR" || { echo "Failed to compile a tester"; exit 1; }
  	echo "Tester is ready"
fi

echo "Starting the test ..."

for INPUTFILE in ../examples/*.csv; do
	if [ -f "$INPUTFILE" ]; then
	
		echo "Checking $INPUTFILE ..."

		../project -c $CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > ./tmp/to_test.txt

		./test_data_validity/tester -c $CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > ./tmp/direct_memory.txt


		sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/to_test.txt

		sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/direct_memory.txt

		sed -i '/Result:/,$d' ./tmp/to_test.txt

		sed -i '/Result:/,$d' ./tmp/direct_memory.txt


		if diff -q ./tmp/to_test.txt ./tmp/direct_memory.txt > /dev/null; then	
			echo -e "${LIGHT_GREEN}\tNo difference between the output with and without cache.${NO_COLOR}"
		else
			../project -c $MANY_CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > ./tmp/to_test.txt

			./test_data_validity/tester -c $MANY_CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > ./tmp/direct_memory.txt


			sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/to_test.txt

			sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/direct_memory.txt

			sed -i '/Result:/,$d' ./tmp/to_test.txt

			sed -i '/Result:/,$d' ./tmp/direct_memory.txt

			# Not enough CYCLES can cause a difference (tester implementation property), making recheck with more CYCLES
			if diff -q ./tmp/to_test.txt ./tmp/direct_memory.txt > /dev/null; then	
				echo -e "${LIGHT_GREEN}\tNo difference between the output with and without cache with larger CYCLES parameter.${NO_COLOR}"
			else
				echo -e "${LIGHT_RED}\tThere is a difference, something is wrong.${NO_COLOR}"
			fi
		fi
	fi
done

echo "The test is over."


rm -r ./tmp/

