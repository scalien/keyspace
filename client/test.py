import keyspace

def genString(x):
	from cStringIO import StringIO
	file_str = StringIO()
	for num in xrange(x):
		file_str.write('$')

	return file_str.getvalue()

def stress(client):
	msg = genString(20*1000) # 20KB values
	
	for num in xrange(10):
		print(str(client.set("big", msg)))

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
	print(str(client.set("user:11", "abeliczay")))
	
	print(str(client.list("user")))
	print(str(client.dirtylist()))
	
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

def protocolEdge(client):
	print(str(client.set(genString(500), "value")))
	print(str(client.set("key", genString(1000*1000))))
	print(str(client.set(genString(1000), genString(1000*1000))))

def protocolError(client):
	print(str(client.set(genString(1001), "value")))
	print(str(client.set("key", genString(1000*1000+1))))

if __name__ == "__main__":
	nodes=["127.0.0.1:7080", "127.0.0.1:7081", "127.0.0.1:7082"]
	client = keyspace.KeyspaceClient(nodes, 2)
	
	client.connectMaster()
	
	#stress(client)
	#users(client)
	#counter(client)
	#hol(client)
	protocolEdge(client)
	#protocolError(client)
	