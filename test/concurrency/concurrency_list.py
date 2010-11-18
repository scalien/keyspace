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
	kvs = client.list_key_values("")
	for key, value in kvs.iteritems():
		if key != value:
			print("TEST FAILED")
			print("%s => %s" % (key, value))
			exit(1)


