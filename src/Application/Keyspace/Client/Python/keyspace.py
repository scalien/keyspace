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
			Keyspace_ResultClose(self.cptr)
			self.cptr = None
		
		def key(self):
			return Keyspace_ResultKey(self.cptr)
		
		def value(self):
			return Keyspace_ResultValue(self.cptr)
		
		def begin(self):
			return Keyspace_ResultBegin(self.cptr)
		
		def is_end(self):
			return Keyspace_ResultIsEnd(self.cptr)
		
		def next(self):
			return Keyspace_ResultNext(self.cptr)
		
		def command_status(self):
			return Keyspace_ResultCommandStatus(self.cptr)
		
		def transport_status(self):
			return Keyspace_ResultTransportStatus(self.cptr)
		
		def connectivity_status(self):
			return Keyspace_ResultConnectivityStatus(self.cptr)
		
		def timeout_status(self):
			return Keyspace_ResultTimeoutStatus(self.cptr)
		
		def key_values(self):
			kv = {}
			self.begin()
			while not self.is_end():
				kv[self.key()] = self.value()
				self.next()
			return kv
	
	def __init__(self, nodes):
		self.co = Keyspace_Create()
		self.result = None
		node_params = Keyspace_NodeParams(len(nodes))
		for node in nodes:
			node_params.AddNode(node)
		Keyspace_Init(self.co, node_params)
		node_params.Close()

	def __del__(self):
		del self.result
		Keyspace_Destroy(self.co)


	def set_global_timeout(self, timeout):
		Keyspace_SetGlobalTimeout(self.co, long(timeout))
	
	def get_global_timeout(self):
		return long(Keyspace_GetGlobalTimeout(self.co))
	
	def set_master_timeout(self, timeout):
		Keyspace_SetMasterTimeout(self.co, long(timeout))
	
	def get_master_timeout(self):
		return long(Keyspace_GetMasterTimeout(self.co))
	
	def get_master(self):
		return Keyspace_GetMaster(self.co)
	
	def get_state(self, node):
		return Keyspace_GetState(self.co, int(node))
	
	def distribute_dirty(self):
		return Keyspace_DistributeDirty(self.co)

	def get(self, key):
		status = Keyspace_Get(self.co, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return self.result.value()

	def dirty_get(self, key):
		status = Keyspace_DirtyGet(self.co, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return self.result.value()

	def count(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_Count(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.co))
		if status < 0:
			return None
		return int(self.result.value())

	def dirty_count(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_DirtyCount(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.co))
		if status < 0:
			return None
		return int(self.result.value())
	
	def list_keys(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_ListKeys(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.co))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def dirty_list_keys(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_DirtyListKeys(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.co))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def list_key_values(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_ListKeyValues(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.co))
		if status < 0:
			return None
		keyvals = {}
		self.result.begin()
		while not self.result.is_end():
			keyvals[self.result.key()] = self.result.value()
			self.result.next()
		return keyvals

	def dirty_list_key_values(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_DirtyListKeyValues(self.co, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.co))
		if status < 0:
			return None
		keyvals = {}
		self.result.begin()
		while not self.result.is_end():
			keyvals[self.result.key()] = self.result.value()
			self.result.next()
		return keyvals
	
	def set(self, key, value):
		status = Keyspace_Set(self.co, key, value)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return status

	def test_and_set(self, key, test, value):
		status = Keyspace_TestAndSet(self.co, key, test, value)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return self.result.value()
	
	def add(self, key, num):
		status = Keyspace_Add(self.co, key, long(num))
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return int(self.result.value())
	
	def delete(self, key):
		status = Keyspace_Delete(self.co, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return status
	
	def remove(self, key):
		status = Keyspace_Remove(self.co, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return self.result.value()
	
	def rename(self, src, dst):
		status = Keyspace_Rename(self.co, src, dst)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return status
	
	def prune(self, prefix):
		status = Keyspace_Prune(self.co, prefix)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.co))
			return
		if Keyspace_IsBatched(self.co):
			return
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return status
	
	def begin(self):
		return Keyspace_Begin(self.co)
	
	def submit(self):
		status = Keyspace_Submit(self.co)
		self.result = Client.Result(Keyspace_GetResult(self.co))
		return status
	
	def cancel(self):
		return Keyspace_Cancel(self.co)
		
	def _set_trace(self, trace):
		Keyspace_SetTrace(trace)

		
