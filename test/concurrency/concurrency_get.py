import keyspace
import random
import common

client = keyspace.Client(["127.0.0.1:7080",
                          "127.0.0.1:7081",
                          "127.0.0.1:7082"])

random.seed()

common.set_print_granularity(1000)

while True:
	common.loop_count()
	i = random.randrange(0, common.num, 1)
	key = "%s" % i
	value = client.get(key)
	if value is None: continue
	if key != value:
		print("TEST FAILED")
		print("%s => %s" % (key, value))
		exit(1)


