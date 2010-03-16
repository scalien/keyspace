from keyspace_client import *

class Client:
	class Result:
		def __init__(self, cptr):
			self.cptr = cptr

		def __del__(self):
			self.close()
		
		def close(self):
			ResultClose(self.cptr)
			self.cptr = None
		
		def key(self):
			return ResultKey(self.cptr)
		
		def value(self):
			return ResultValue(self.cptr)
		
		def begin(self):
			return ResultBegin(self.cptr)
		
		def is_end(self):
			return ResultIsEnd(self.cptr)
		
		def next(self):
			return ResultNext(self.cptr)
		
		def command_status(self):
			return ResultCommandStatus(self.cptr)
		
		def transport_status(self):
			return ResultTransportStatus(self.cptr)
		
		def connectivity_status(self):
			return ResultConnectivityStatus(self.cptr)
		
		def timeout_status(self):
			return ResultTimeoutStatus(self.cptr)
	
	def __init__(self, nodes):
		self.co = Create()
		self.result = None
		node_params = NodeParams(len(nodes))
		for node in nodes:
			node_params.AddNode(node)
		Init(self.co, node_params)
		node_params.Close()

	def get(self, key):
		status = Get(self.co, key)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return self.result.value()

	def dirty_get(self, key):
		status = DirtyGet(self.co, key)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return self.result.value()

	def count(self, prefix = "", start_key = "", count = 0, next = False, forward = True):
		status = Count(self.co, prefix, start_key, count, next, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		return int(self.result.value())

	def dirty_count(self, prefix = "", start_key = "", count = 0, next = False, forward = True):
		status = DirtyCount(self.co, prefix, start_key, count, next, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		return int(self.result.value())
	
	def list_keys(self, prefix = "", start_key = "", count = 0, next = False, forward = True):
		status = ListKeys(self.co, prefix, start_key, count, next, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def dirty_list_keys(self, prefix = "", start_key = "", count = 0, next = False, forward = True):
		status = DirtyListKeys(self.co, prefix, start_key, count, next, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def list_key_values(self, prefix = "", start_key = "", count = 0, next = False, forward = True):
		status = ListKeyValues(self.co, prefix, start_key, count, next, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		keyvals = {}
		self.result.begin()
		while not self.result.is_end():
			keyvals[self.result.key()] = self.result.value()
			self.result.next()
		return keyvals

	def dirty_list_key_values(self, prefix = "", start_key = "", count = 0, next = False, forward = True):
		status = DirtyListKeyValues(self.co, prefix, start_key, count, next, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		keyvals = {}
		self.result.begin()
		while not self.result.is_end():
			keyvals[self.result.key()] = self.result.value()
			self.result.next()
		return keyvals
	
	def set(self, key, value):
		status = Set(self.co, key, value)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return status

	def test_and_set(self, key, test, value):
		status = TestAndSet(self.co, key, test, value)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return self.result.value()
	
	def add(self, key, num):
		status = Add(self.co, key, long(num))
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return int(self.result.value())
	
	def delete(self, key):
		status = Delete(self.co, key)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return status
	
	def remove(self, key):
		status = Remove(self.co, key)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return self.result.value()
	
	def rename(self, src, dst):
		status = Rename(self.co, src, dst)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return status
	
	def prune(self, prefix):
		status = Prune(self.co, prefix)
		if status < 0:
			self.result = Client.Result(GetResult(self.co))
			return
		if IsBatched(self.co):
			return
		self.result = Client.Result(GetResult(self.co))
		return status
	
	def begin(self):
		return Begin(self.co)
	
	def submit(self):
		return Submit(self.co)
	
	def cancel(self):
		return Cancel(self.co)
	
	
		
