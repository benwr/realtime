for i in `seq 65 10 250`; do
	sched_test_app -s LBESA -c $i -r 15 -f 10t_nl -t timer
done
