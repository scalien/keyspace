#include "KeyspaceClientWrap.h"
#include "KeyspaceClient.h"

/////////////////////////////////////////////////////////////////////
//
// NodeParams implementation
//
/////////////////////////////////////////////////////////////////////
Keyspace_NodeParams::Keyspace_NodeParams(int nodec_)
{
	num = 0;
	nodec = nodec_;
	nodes = new char*[nodec];
	for (int i = 0; i < nodec; i++)
		nodes[i] = NULL;
}
	
Keyspace_NodeParams::~Keyspace_NodeParams()
{
	Close();
}
	
void Keyspace_NodeParams::Close()
{
	for (int i = 0; i < num; i++)
		free(nodes[i]);
	delete[] nodes;
	nodes = NULL;
	num = 0;
}
	
void Keyspace_NodeParams::AddNode(const std::string& node)
{
	if (num > nodec)
		return;
	nodes[num++] = strdup(node.c_str());
}

/////////////////////////////////////////////////////////////////////
//
// Result functions
//
/////////////////////////////////////////////////////////////////////
void Keyspace_ResultBegin(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;

	if (result)		
		result->Begin();
}

void Keyspace_ResultNext(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	
	if (result)
		result->Next();	
}

bool Keyspace_ResultIsEnd(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	
	if (!result)
		return true;

	return result->IsEnd();
}

void Keyspace_ResultClose(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	
	if (result)
		result->Close();
	delete result;
}

std::string Keyspace_ResultKey(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	ByteString			key;
	int					status;
	std::string			ret;
	
	if (!result)
		return ret;
	
	status = result->Key(key);
	if (status < 0)
		return ret;
	
	ret.append(key.buffer, key.length);
	
	return ret;
}

std::string Keyspace_ResultValue(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	ByteString			value;
	int					status;
	std::string			ret;

	if (!result)
		return ret;
	
	status = result->Value(value);
	if (status < 0)
		return ret;
	
	ret.append(value.buffer, value.length);

	return ret;
}

int	Keyspace_ResultTransportStatus(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;

	if (!result)
		return KEYSPACE_API_ERROR;
	
	return result->TransportStatus();
}

int	Keyspace_ResultConnectivityStatus(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;

	if (!result)
		return KEYSPACE_API_ERROR;
	
	return result->ConnectivityStatus();
}

int Keyspace_ResultTimeoutStatus(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;

	if (!result)
		return KEYSPACE_API_ERROR;
	
	return result->TimeoutStatus();

}

int Keyspace_ResultCommandStatus(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;

	if (!result)
		return KEYSPACE_API_ERROR;
	
	return result->CommandStatus();
}

/////////////////////////////////////////////////////////////////////
//
// Client functions
//
/////////////////////////////////////////////////////////////////////
ClientObj Keyspace_Create()
{
	return new Keyspace::Client();
}

int	Keyspace_Init(ClientObj client_, const Keyspace_NodeParams& params)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	int					status;

	status = client->Init(params.nodec, (const char**)params.nodes);
	
	return status;
}

void Keyspace_Destroy(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;

	delete client;
}

ResultObj Keyspace_GetResult(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
		
	return client->GetResult();
}

void Keyspace_SetGlobalTimeout(ClientObj client_, uint64_t timeout)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;

	client->SetGlobalTimeout(timeout);
}

void Keyspace_SetMasterTimeout(ClientObj client_, uint64_t timeout)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;

	client->SetMasterTimeout(timeout);
}

uint64_t Keyspace_GetGlobalTimeout(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->GetGlobalTimeout();
}

uint64_t Keyspace_GetMasterTimeout(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->GetMasterTimeout();
}

int Keyspace_GetMaster(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->GetMaster();
}

void Keyspace_DistributeDirty(ClientObj client_, bool dd)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;

	client->DistributeDirty(dd);
}

int Keyspace_Get(ClientObj client_, const std::string& key_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	
	return client->Get(key);
}

int Keyspace_DirtyGet(ClientObj client_, const std::string& key_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	
	return client->DirtyGet(key);
}

int Keyspace_Count(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  uint64_t count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	uint64_t			res;
		
	return client->Count(res, prefix, startKey, count_, next_, forward_);
}

int Keyspace_CountStr(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  const std::string& count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	uint64_t			res;
	unsigned			read;
	uint64_t			count;

	count = strntouint64(count_.c_str(), count_.length(), &read);
	if (read != count_.length())
		return KEYSPACE_API_ERROR;
		
	return client->Count(res, prefix, startKey, count, next_, forward_);
}

int Keyspace_DirtyCount(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  uint64_t count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	uint64_t			res;
		
	return client->DirtyCount(res, prefix, startKey, count_, next_, forward_);
}

int Keyspace_DirtyCountStr(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  const std::string& count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	uint64_t			res;
	unsigned			read;
	uint64_t			count;

	count = strntouint64(count_.c_str(), count_.length(), &read);
	if (read != count_.length())
		return KEYSPACE_API_ERROR;
		
	return client->DirtyCount(res, prefix, startKey, count, next_, forward_);
}

int	Keyspace_ListKeys(ClientObj client_, 
			 const std::string& prefix_,
			 const std::string& startKey_,
			 uint64_t count_,
			 bool next_,
			 bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
		
	return client->ListKeys(prefix, startKey, count_, next_, forward_);
}

int Keyspace_ListKeysStr(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  const std::string& count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	unsigned			read;
	uint64_t			count;

	count = strntouint64(count_.c_str(), count_.length(), &read);
	if (read != count_.length())
		return KEYSPACE_API_ERROR;
		
	return client->ListKeys(prefix, startKey, count, next_, forward_);
}

int	Keyspace_DirtyListKeys(ClientObj client_, 
			 const std::string& prefix_,
			 const std::string& startKey_,
			 uint64_t count_,
			 bool next_,
			 bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
		
	return client->DirtyListKeys(prefix, startKey, count_, next_, forward_);
}

int Keyspace_DirtyListKeysStr(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  const std::string& count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	unsigned			read;
	uint64_t			count;

	count = strntouint64(count_.c_str(), count_.length(), &read);
	if (read != count_.length())
		return KEYSPACE_API_ERROR;
		
	return client->DirtyListKeys(prefix, startKey, count, next_, forward_);
}

int	Keyspace_ListKeyValues(ClientObj client_, 
			 const std::string& prefix_,
			 const std::string& startKey_,
			 uint64_t count_,
			 bool next_,
			 bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
		
	return client->ListKeyValues(prefix, startKey, count_, next_, forward_);
}

int Keyspace_ListKeyValuesStr(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  const std::string& count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	unsigned			read;
	uint64_t			count;

	count = strntouint64(count_.c_str(), count_.length(), &read);
	if (read != count_.length())
		return KEYSPACE_API_ERROR;
		
	return client->ListKeyValues(prefix, startKey, count, next_, forward_);
}

int	Keyspace_DirtyListKeyValues(ClientObj client_, 
			 const std::string& prefix_,
			 const std::string& startKey_,
			 uint64_t count_,
			 bool next_,
			 bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
		
	return client->DirtyListKeyValues(prefix, startKey, count_, next_, forward_);
}

int Keyspace_DirtyListKeyValuesStr(ClientObj client_, 
				  const std::string& prefix_,
				  const std::string& startKey_,
				  const std::string& count_,
				  bool next_,
				  bool forward_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	ByteString			startKey(startKey_.length(), startKey_.length(), startKey_.c_str());
	unsigned			read;
	uint64_t			count;

	count = strntouint64(count_.c_str(), count_.length(), &read);
	if (read != count_.length())
		return KEYSPACE_API_ERROR;
		
	return client->DirtyListKeyValues(prefix, startKey, count, next_, forward_);
}


int Keyspace_Set(ClientObj client_, const std::string& key_, const std::string& value_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	ByteString			value(value_.length(), value_.length(), value_.c_str());

	return client->Set(key, value);
}

int Keyspace_TestAndSet(ClientObj client_, 
			   const std::string& key_,
			   const std::string& test_,
			   const std::string& value_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	ByteString			test(test_.length(), test_.length(), test_.c_str());
	ByteString			value(value_.length(), value_.length(), value_.c_str());

	return client->TestAndSet(key, test, value);
}

int Keyspace_Add(ClientObj client_, const std::string& key_, int64_t num_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	int64_t				result;
	
	return client->Add(key, num_, result);
}

int Keyspace_AddStr(ClientObj client_, const std::string& key_, const std::string& num_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	int64_t				result;
	unsigned			read;
	int64_t				num;

	num = strntoint64(num_.c_str(), num_.length(), &read);
	if (read != num_.length())
		return KEYSPACE_API_ERROR;
	
	return client->Add(key, num, result);
}

int Keyspace_Delete(ClientObj client_, const std::string& key_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	
	return client->Delete(key); 
}

int Keyspace_Remove(ClientObj client_, const std::string& key_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	
	return client->Remove(key); 
}

int	Keyspace_Rename(ClientObj client_, const std::string& from_, const std::string& to_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			from(from_.length(), from_.length(), from_.c_str());
	ByteString			to(to_.length(), to_.length(), to_.c_str());
	
	return client->Rename(from, to); 
}

int Keyspace_Prune(ClientObj client_, const std::string& prefix_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	
	return client->Prune(prefix);
}

int Keyspace_Begin(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->Begin();
}

int Keyspace_Submit(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->Submit();
}

int Keyspace_Cancel(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->Cancel();
}

bool Keyspace_IsBatched(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->IsBatched();
}

void Keyspace_SetTrace(bool trace)
{
	if (trace)
	{
		Log_SetTrace(true);
		Log_SetTarget(LOG_TARGET_STDOUT);
	}
	else
	{
		Log_SetTrace(false);
		Log_SetTarget(LOG_TARGET_NOWHERE);
	}
}

