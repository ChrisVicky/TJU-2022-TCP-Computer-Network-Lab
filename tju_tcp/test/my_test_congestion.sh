#!/bin/bash 
iter=(8 16 24 32 40 48 56 64)
for i in ${iter[@]}
do
	python3 my_test_congestion.py 50 6 0 0 $i
	python3 gen_graph_seq.py
	python3 gen_graph_win.py $i
done
