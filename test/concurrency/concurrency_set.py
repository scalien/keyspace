import keyspace
import random
import common

client = keyspace.Client(["127.0.0.1:7080",
                          "127.0.0.1:7081",
                          "127.0.0.1:7082"])

random.seed()

common.set_print_granularity(100)

while True:
	common.loop_count()
	i = random.randrange(0, common.num, 1)
	key = "%s" % i
	value = key
	client.set(key, value)


