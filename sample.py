from rex import Rex

rex = Rex("100113_SO1")

# print the first trial
print "The first trial is: "
print rex.trials[0]

# print the first event in the first trial
print "The first event in the first trial is: "
print rex.trials[0].events[0]


