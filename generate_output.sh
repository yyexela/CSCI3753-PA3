#!/bin/bash

make clean;
clear;
make;
echo "-----------"

./multi-lookup 1 1 serviced.txt results.txt input/names*.txt > temp_output.txt
head -2 temp_output.txt > performance.txt
grep Thread serviced.txt >> performance.txt
tail -1 temp_output.txt >> performance.txt

./multi-lookup 1 3 serviced.txt results.txt input/names*.txt > temp_output.txt
head -2 temp_output.txt >> performance.txt
grep Thread serviced.txt >> performance.txt
tail -1 temp_output.txt >> performance.txt

./multi-lookup 3 1 serviced.txt results.txt input/names*.txt > temp_output.txt
head -2 temp_output.txt >> performance.txt
grep Thread serviced.txt >> performance.txt
tail -1 temp_output.txt >> performance.txt

./multi-lookup 3 3 serviced.txt results.txt input/names*.txt > temp_output.txt
head -2 temp_output.txt >> performance.txt
grep Thread serviced.txt >> performance.txt
tail -1 temp_output.txt >> performance.txt

./multi-lookup 5 5 serviced.txt results.txt input/names*.txt > temp_output.txt
head -2 temp_output.txt >> performance.txt
grep Thread serviced.txt >> performance.txt
tail -1 temp_output.txt >> performance.txt

