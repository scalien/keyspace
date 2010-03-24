from keyspace_client import *

KEYSPACE_SUCCESS = 0
KEYSPACE_API_ERROR = -1

KEYSPACE_PARTIAL = -101
KEYSPACE_FAILURE = -102

KEYSPACE_NOMASTER = -201
KEYSPACE_NOCONNECTION =	-202

KEYSPACE_MASTER_TIMEOUT = -301
KEYSPACE_GLOBAL_TIMEOUT = -302

KEYSPACE_NOSERVICE = -401
KEYSPACE_FAILED = -402

def str_status(status):
	if status == KEYSPACE_SUCCESS:
		return "KEYSPACE_SUCCESS"
	elif status == KEYSPACE_API_ERROR:
		return "KEYSPACE_API_ERROR"
	elif status == KEYSPACE_PARTIAL:
		return "KEYSPACE_PARTIAL"
	elif status == KEYSPACE_FAILURE:
		return "KEYSPACE_FAILURE"
	elif status == KEYSPACE_NOMASTER:
		return "KEYSPACE_NOMASTER"
	elif status == KEYSPACE_NOCONNECTION:
		return "KEYSPACE_NOCONNECTION"
	elif status == KEYSPACE_MASTER_TIMEOUT:
		return "KEYSPACE_MASTER_TIMEOUT"
	elif status == KEYSPACE_GLOBAL_TIMEOUT:
		return "KEYSPACE_GLOBAL_TIMEOUT"
	elif status == KEYSPACE_NOSERVICE:
		return "KEYSPACE_NOSERVICE"
	elif status == KEYSPACE_FAILED:
		return "KEYSPACE_FAILED"

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
		
		def key_values(self):
			kv = {}
			self.begin()
			while not self.is_end():
				kv[self.key()] = self.value()
				self.next()
			return kv
	
	def __init__(self, nodes):
		self.co = Create()
		self.result = None
		node_params = NodeParams(len(nodes))
		for node in nodes:
			node_params.AddNode(node)
		Init(self.co, node_params)
		node_params.Close()

	def __del__(self):
		del self.result
		Destroy(self.co)


	def set_global_timeout(self, timeout):
		SetGlobalTimeout(self.co, long(timeout))
	
	def get_global_timeout(self):
		return long(GetGlobalTimeout(self.co))
	
	def set_master_timeout(self, timeout):
		SetMasterTimeout(self.co, long(timeout))
	
	def get_master_timeout(self):
		return long(GetMasterTimeout(self.co))
	
	def get_master(self):
		return GetMaster(self.co)
	
	def get_state(self, node):
		return GetState(self.co, int(node))
	
	def distribute_dirty(self):
		return DistributeDirty(self.co)

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

	def count(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Count(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		return int(self.result.value())

	def dirty_count(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = DirtyCount(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		return int(self.result.value())
	
	def list_keys(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = ListKeys(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def dirty_list_keys(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = DirtyListKeys(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def list_key_values(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = ListKeyValues(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(GetResult(self.co))
		if status < 0:
			return None
		keyvals = {}
		self.result.begin()
		while not self.result.is_end():
			keyvals[self.result.key()] = self.result.value()
			self.result.next()
		return keyvals

	def dirty_list_key_values(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = DirtyListKeyValues(self.co, prefix, start_key, count, skip, forward)
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
		status = Submit(self.co)
		self.result = Client.Result(GetResult(self.co))
		return status
	
	def cancel(self):
		return Cancel(self.co)
		
	def _set_trace(self, trace):
		SetTrace(trace)

		
