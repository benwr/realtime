result = []
while True:
	try:
		input = raw_input()
	except EOFError:
		break
	
	cpu, rest = input.split("/", 1)

	algo, rest = rest.split(": ", 1)

	_, rest = rest.split(": ", 1)

	tasks, rest = rest.split("/", 1)
	tottasks, rest = rest.split(",", 1)

	_, rest = rest.split(": ", 1)

	util, rest = rest.split("/", 1)
	totutil, rest = rest.split(",", 1)
	
	_, rest = rest.split(": ", 1)
	
	aborted, rest = rest.split(",", 1)
	print "{0},{1},{2},{3},{4},{5},{6}".format(algo, cpu, tasks, tottasks, util,totutil, aborted)

	result.append((algo, float(cpu), int(tasks), int(tottasks), int(util), int(totutil), int(aborted)))

#print result



