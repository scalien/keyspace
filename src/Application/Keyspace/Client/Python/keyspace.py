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
		self.cptr = Keyspace_Create()
		self.result = None
		node_params = Keyspace_NodeParams(len(nodes))
		for node in nodes:
			node_params.AddNode(node)
		Keyspace_Init(self.cptr, node_params)
		node_params.Close()

	def __del__(self):
		del self.result
		Keyspace_Destroy(self.cptr)


	def set_global_timeout(self, timeout):
		Keyspace_SetGlobalTimeout(self.cptr, long(timeout))
	
	def get_global_timeout(self):
		return long(Keyspace_GetGlobalTimeout(self.cptr))
	
	def set_master_timeout(self, timeout):
		Keyspace_SetMasterTimeout(self.cptr, long(timeout))
	
	def get_master_timeout(self):
		return long(Keyspace_GetMasterTimeout(self.cptr))
	
	def get_master(self):
		return Keyspace_GetMaster(self.cptr)
	
	def get_state(self, node):
		return Keyspace_GetState(self.cptr, int(node))
	
	def distribute_dirty(self):
		return Keyspace_DistributeDirty(self.cptr)

	def get(self, key):
		status = Keyspace_Get(self.cptr, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return self.result.value()

	def dirty_get(self, key):
		status = Keyspace_DirtyGet(self.cptr, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return self.result.value()

	def count(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_Count(self.cptr, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		if status < 0:
			return None
		return int(self.result.value())

	def dirty_count(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_DirtyCount(self.cptr, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		if status < 0:
			return None
		return int(self.result.value())
	
	def list_keys(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_ListKeys(self.cptr, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def dirty_list_keys(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_DirtyListKeys(self.cptr, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		if status < 0:
			return None
		keys = []
		self.result.begin()
		while not self.result.is_end():
			keys.append(self.result.key())
			self.result.next()
		return keys
	
	def list_key_values(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_ListKeyValues(self.cptr, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		if status < 0:
			return None
		keyvals = {}
		self.result.begin()
		while not self.result.is_end():
			keyvals[self.result.key()] = self.result.value()
			self.result.next()
		return keyvals

	def dirty_list_key_values(self, prefix = "", start_key = "", count = 0, skip = False, forward = True):
		status = Keyspace_DirtyListKeyValues(self.cptr, prefix, start_key, count, skip, forward)
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		if status < 0:
			return None
		keyvals = {}
		self.result.begin()
		while not self.result.is_end():
			keyvals[self.result.key()] = self.result.value()
			self.result.next()
		return keyvals
	
	def set(self, key, value):
		status = Keyspace_Set(self.cptr, key, value)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return status

	def test_and_set(self, key, test, value):
		status = Keyspace_TestAndSet(self.cptr, key, test, value)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return self.result.value()
	
	def add(self, key, num):
		status = Keyspace_Add(self.cptr, key, long(num))
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return int(self.result.value())
	
	def delete(self, key):
		status = Keyspace_Delete(self.cptr, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return status
	
	def remove(self, key):
		status = Keyspace_Remove(self.cptr, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return self.result.value()
	
	def rename(self, src, dst):
		status = Keyspace_Rename(self.cptr, src, dst)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return status
	
	def prune(self, prefix):
		status = Keyspace_Prune(self.cptr, prefix)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return status

	def set_expiry(self, key, exptime):
		status = Keyspace_SetExpiry(self.cptr, key, exptime)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return status
	
	def remove_expiry(self, key):
		status = Keyspace_RemoveExpiry(self.cptr, key)
		if status < 0:
			self.result = Client.Result(Keyspace_GetResult(self.cptr))
			return
		if Keyspace_IsBatched(self.cptr):
			return
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return status
	
	def begin(self):
		return Keyspace_Begin(self.cptr)
	
	def submit(self):
		status = Keyspace_Submit(self.cptr)
		self.result = Client.Result(Keyspace_GetResult(self.cptr))
		return status
	
	def cancel(self):
		return Keyspace_Cancel(self.cptr)
		
	def _set_trace(self, trace):
		Keyspace_SetTrace(trace)

		
