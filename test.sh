#!/bin/bash

make clean;
clear;
make;
echo "-----------"
./multi-lookup 1 1 serviced.txt results.txt "input/names1.txt"

#if test -x "multi-lookup"; then
#	./multi-lookup 5 5 req_log.txt res_log.txt
#else 
#	echo "multi-lookup doesn't exist"
#fi
