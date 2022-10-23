for ((i=0; i<100; i+=10))
do
	for ((k=0;k<=100;k+=20))
	do
		for ((j=0;j<=k;j+=15))
		do
			for ((w=100;w<=1000;w+=100))
			do
				echo python3 test_congestion.py $w $k $j $i
				python3 test_congestion.py $w $k $j $i
				echo python3 gen_graph_seq.py
				python3 gen_graph_seq.py
				echo python3 gen_graph_win.py
				python3 gen_graph_win.py
			done
		done
	done
done
