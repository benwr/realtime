for i in `seq 65 10 250`; do
	sched_test_app -s DASA_ND -c $i -r 60 -f 10t_nl -t timer
	sched_test_app -s EDF -c $i -r 60 -f 10t_nl -t timer
	sched_test_app -s RMA -c $i -r 60 -f 10t_nl -t timer
done
