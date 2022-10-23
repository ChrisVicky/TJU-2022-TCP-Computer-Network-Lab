#!/bin/bash 
python3 my_test_congestion.py $1 $2 $3 $4
python3 gen_graph_seq.py
python3 gen_graph_win.py
