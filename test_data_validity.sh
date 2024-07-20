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


if [ ! -d /tmp/test_data_validity/ ]; then
  mkdir /tmp/test_data_validity/
fi


for INPUTFILE in *.csv; do
	if [ -f "$INPUTFILE" ]; then
	
		echo "Checking $INPUTFILE ..."

		./project -c $CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > /tmp/test_data_validity/to_test.txt

		./test_data_validity/project -c $CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > /tmp/test_data_validity/direct_memory.txt


		sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' /tmp/test_data_validity/to_test.txt

		sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' /tmp/test_data_validity/direct_memory.txt

		sed -i '/Result:/,$d' /tmp/test_data_validity/to_test.txt

		sed -i '/Request:/d' /tmp/test_data_validity/direct_memory.txt

		sed -i '/Result:/,$d' /tmp/test_data_validity/direct_memory.txt


		if diff -q /tmp/test_data_validity/to_test.txt /tmp/test_data_validity/direct_memory.txt > /dev/null; then	
			echo -e "${LIGHT_GREEN}\tNo difference between the output with and without cache.${NO_COLOR}"
		else
			./project -c $MANY_CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > /tmp/test_data_validity/to_test.txt

			./test_data_validity/project -c $MANY_CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > /tmp/test_data_validity/direct_memory.txt


			sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' /tmp/test_data_validity/to_test.txt

			sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' /tmp/test_data_validity/direct_memory.txt

			sed -i '/Result:/,$d' /tmp/test_data_validity/to_test.txt

			sed -i '/Result:/,$d' /tmp/test_data_validity/direct_memory.txt


			if diff -q /tmp/test_data_validity/to_test.txt /tmp/test_data_validity/direct_memory.txt > /dev/null; then	
				echo -e "${MAGENTA}\tThere is a difference, but it was liquidated after using larger CYCLES parameter.${NO_COLOR}"
			else
				echo -e "${LIGHT_RED}\tThere is a difference, something is wrong.${NO_COLOR}"
			fi
		fi
	fi
done

echo "The test is over."


#rm -r /tmp/test_data_validity/

