#!/bin/bash

make clean;
clear;
make;
echo "-----------"
gdb --args multi-lookup 5 1 serviced.txt results.txt "input/names1.txt" "input/names2.txt" "input/names3.txt" "input/names4.txt"

#if test -x "multi-lookup"; then
#	./multi-lookup 5 5 req_log.txt res_log.txt
#else 
#	echo "multi-lookup doesn't exist"
#fi
