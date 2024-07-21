#!/bin/bash

MANY_CYCLES=2000000000

LIGHT_GREEN='\033[1;32m'
LIGHT_RED='\033[1;31m'
LIGHT_BLUE='\033[1;34m'
MAGENTA='\033[1;35m'
NO_COLOR='\033[0m'

BOOL_DIFF=false

echo -en "${LIGHT_BLUE}Number of random test iterations: ${NO_COLOR}"
read ITER

if [ ! -d ./tmp/ ]; then
	  mkdir ./tmp/
fi

#Compiling the tester (if not already)
if [ ! -f "./test_data_validity/tester" ]; then
	echo "Compiling the tester ..."
  	TESTER_DIR="./test_data_validity"
  	make -C "$TESTER_DIR" || { echo "Failed to make a tester"; exit 1; }
  	echo "Tester is ready"
fi
for i in $(seq 1 "$ITER")
do

	#Because of performance issues the decision to additionally limit parameteres was made
	
	CYCLES=$(( (RANDOM * RANDOM) % 100000000 ))

	LINESIZE=$(( 2 ** (2 + RANDOM % 13) ))

	L2LINES=$(( 2 ** (2 + RANDOM % 13) ))
	
	L1LINES=$(( 2 ** (2 + RANDOM % 13) ))
	if [ $L1LINES -gt $L2LINES ]; then
		L1LINES=$L2LINES
	fi	
	
	MEMLAT=$(( (RANDOM * RANDOM) % 100000 ))
	L2LAT=$(( RANDOM * RANDOM % (MEMLAT + 1) ))
	L1LAT=$(( RANDOM * RANDOM % (L2LAT + 1) ))
	
	for INPUTFILE in ../examples/*.csv; do
		if [ -f "$INPUTFILE" ]; then
		
			../project -c $CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > ./tmp/to_test.txt

			./test_data_validity/tester -c $CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > ./tmp/direct_memory.txt


			sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/to_test.txt
	
			sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/direct_memory.txt

			sed -i '/Result:/,$d' ./tmp/to_test.txt

			sed -i '/Result:/,$d' ./tmp/direct_memory.txt


			if ! diff -q ./tmp/to_test.txt ./tmp/direct_memory.txt > /dev/null; then
				
				../project -c $MANY_CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency $L1LAT --l2-latency $L2LAT --memory-latency $MEMLAT $INPUTFILE > ./tmp/to_test.txt

				./test_data_validity/tester -c $MANY_CYCLES --cacheline-size $LINESIZE --l1-lines $L1LINES --l2-lines $L2LINES --l1-latency 0 --l2-latency 0 --memory-latency 0 $INPUTFILE > ./tmp/direct_memory.txt


				sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/to_test.txt

				sed -i '0,/Info: \/OSCI\/SystemC: Simulation stopped by user\./d' ./tmp/direct_memory.txt

				sed -i '/Result:/,$d' ./tmp/to_test.txt

				sed -i '/Result:/,$d' ./tmp/direct_memory.txt


				# Not enough CYCLES can cause a difference (tester implementation property), making recheck with more CYCLES
				if ! diff -q ./tmp/to_test.txt ./tmp/direct_memory.txt > /dev/null; then
					echo -e "${LIGHT_RED}There is a difference, something is wrong.${NO_COLOR} Input infomation:"
					BOOL_DIFF=true
								
					echo -e "\t${LIGHT_BLUE}Cycles: ${NO_COLOR}$CYCLES"
					echo -e "\t${LIGHT_BLUE}Linesize: ${NO_COLOR}$LINESIZE"
					echo -e "\t${LIGHT_BLUE}L1 cachelines: ${NO_COLOR}$L1LINES"
					echo -e "\t${LIGHT_BLUE}L2 cachelines: ${NO_COLOR}$L2LINES"
					echo -e "\t${LIGHT_BLUE}L1 latency: ${NO_COLOR}$L1LAT"
					echo -e "\t${LIGHT_BLUE}L2 latency: ${NO_COLOR}$L2LAT"
					echo -e "\t${LIGHT_BLUE}Memory latency: ${NO_COLOR}$MEMLAT"
					echo -e "\t${LIGHT_BLUE}File: ${NO_COLOR}$INPUTFILE"
				fi
			fi
		fi
	done

done

echo "The test is over."

if [ $BOOL_DIFF = false ]; then
	echo -e "${LIGHT_GREEN}No serious problems found${NO_COLOR}"
fi

rm -r ./tmp/
