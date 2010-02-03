#!/usr/bin/env python

import time
import keyspace

def genString(x):
	from cStringIO import StringIO
	file_str = StringIO()
	for num in xrange(x):
		file_str.write('$')

	return file_str.getvalue()

def stress(client):
	msg = genString(1000*1000)

	print("Starting Keyspace run...")
	
	for num in xrange(100):
		print(str(client.set("big" + str(num), msg)))

def users(client):
	print(str(client.set("user:0", "mtrencseni")))
	print(str(client.set("user:1", "agazso")))
	print(str(client.set("user:2", "pjozsa")))
	print(str(client.set("user:3", "dtorok")))
	print(str(client.set("user:4", "pbagi")))
	print(str(client.set("user:5", "zsszabo")))
	print(str(client.set("user:6", "koszko")))
	print(str(client.set("user:7", "itakacs")))
	print(str(client.set("user:8", "akutasi")))
	print(str(client.set("user:9", "tmatrahegyi")))
	print(str(client.set("user:10", "pfarago")))
	print(str(client.set("user:11", "glaszlo")))
	print(str(client.set("user:12", "abeliczay")))
	
	# print(str(client.listkeys("user")))
	# print(str(client.dirtylistkeys()))

def nset(client, num, value = "0123456789"):
	for i in xrange(num):
		try:
			client.set("x:%d" % i, value)
		except keyspace.NotMasterException, e:
			print("NotMasterException")
			# time.sleep(10)
	
def hol(client):
	print(str(client.set("hol","peru")))
	print(str(client.get("hol")))
	print(str(client.dirtyget("hol")))
	print(str(client.testandset("hol","peru", "budapest")))
	print(str(client.get("hol")))
	print(str(client.delete("hol")))
	print(str(client.get("hol")))

def counter(client):
	print(str(client.set("counter", 0)))
	print(str(client.get("counter")))
	print(str(client.add("counter", 100)))
	print(str(client.get("counter")))
	print(str(client.add("counter", -100)))
	print(str(client.get("counter")))
	print(str(client.add("counter", 2**32)))
	print(str(client.get("counter")))

def elapsedGet(client):
	starttime = time.time()
	client.get("counter")
	endtime = time.time()
	elapsed = endtime - starttime
	print("elapsedGet: %f" % elapsed)
	return elapsed
	
def elapsedSet(client):
	starttime = time.time()
	client.set("counter", 0)
	endtime = time.time()
	elapsed = endtime - starttime
	print("elapsedGet: %f" % elapsed)
	return elapsed	

def protocolEdge(client):
	print(str(client.set(genString(1000), "value")))
	print(str(client.set("key", genString(1000*1000))))
	print(str(client.set(genString(1000), genString(1000*1000))))

def protocolError(client):
	#print(str(client.set(genString(1000+1), "value")))
	print(str(client.set("key", genString(1000*1000+1))))

def submitTest(client):
	client.begin()
	for i in xrange(10000):
		client.set("test" + str(i), i, False)
	client.submit()
	
def benchmarkSet(client):
	nset = 100000
	starttime = time.time()

	client.begin()
	for i in xrange(nset):
		client.set("test" + str(i), i, False)
	client.submit()

	endtime = time.time()
	elapsed = endtime - starttime
	print("benchmarkSet: %f/s" % (float(nset) / elapsed))
	

if __name__ == "__main__":
	nodes=["127.0.0.1:7080","127.0.0.1:7081","127.0.0.1:7082"]
	#nodes = ["192.168.1.50:7080"]
	client = keyspace.Client(nodes, 5, True)
	
	#starttime = time.time()

	client.prune("")
	# stress(client)
	#client.prune("")
	# submitTest(client)
	#users(client)
	nset(client, 100)

	#endtime = time.time()
	#elapsed = endtime - starttime
	#print("elapsedGet: %f" % elapsed)
	
	benchmarkSet(client)
	#users(client)
	#counter(client)
	#hol(client)
	#protocolEdge(client)
	#protocolError(client)	
