for i in `seq 65 250`; do
	sched_test_app -s DASA_ND -c $i -r 10 -f 5t_nl -t timer
	sched_test_app -s EDF -c $i -r 10 -f 5t_nl -t timer
	sched_test_app -s RMA -c $i -r 10 -f 5t_nl -t timer
done
