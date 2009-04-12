import keyspace

def genBig(x):
	from cStringIO import StringIO
	file_str = StringIO()
	for num in xrange(x):
		file_str.write('$')

	return file_str.getvalue()

def setBigVal(client):
	client.connectMaster()
	
	msg = genBig(50000)
	
	client.set("big", msg)
	

if __name__ == "__main__":
	nodes=["localhost:7080", "localhost:7081"]
	client = keyspace.KeyspaceClient(nodes)
	setBigVal(client)

	exit()

	client.connectMaster()

	# resp = client.dirtylist("", 0)
	# print(str(resp))
	

	resp = client.dirtyget("counter")
	print(str(resp))
	resp = client.dirtyget("counter")
	print(str(resp))

	resp = client.set("hol", "peru")
	print(str(resp))
	
	resp = client.dirtyget("hol")
	print(str(resp))

	resp = client.dirtyget("hol")
	print(str(resp))

	resp = client.testandset("hol", "peru", "budapest")
	print(str(resp))

	resp = client.get("hol")
	print(str(resp))

	resp = client.delete("hol")
	print(str(resp))

	resp = client.get("hol")
	print(str(resp))

	resp = client.set("counter", 0)
	print(str(resp))

	resp = client.add("counter", 1)
	print(str(resp))

	resp = client.add("counter", 1)
	print(str(resp))

	#~ resp = client.list("", 10)
	#~ print(str(resp))
