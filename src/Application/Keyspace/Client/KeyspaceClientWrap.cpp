#include "KeyspaceClientWrap.h"
#include "KeyspaceClient.h"

void ResultBegin(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;

	if (result)		
		result->Begin();
}

void ResultNext(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	
	if (result)
		result->Next();	
}

bool ResultIsEnd(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	
	if (!result)
		return true;

	return result->IsEnd();
}

void ResultClose(ResultObj result_)
{
	Keyspace::Result*	result = (Keyspace::Result*) result_;
	
	if (result)
		result->Close();
}

std::string ResultKey(ResultObj result_)
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

std::string ResultValue(ResultObj result_)
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

ClientObj Create()
{
	return new Keyspace::Client();
}

int	Init(ClientObj client_, const NodeParams& params)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	int					status;

	Log_SetTrace(true);
	Log_SetTarget(LOG_TARGET_STDOUT);
	status = client->Init(params.nodec, params.nodes);
	
	return status;
}

ResultObj GetResult(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
		
	return client->GetResult();
}

int Get(ClientObj client_, const std::string& key_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	
	return client->Get(key);
}

uint64_t Count(ClientObj client_, 
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
		
	client->Count(res, prefix, startKey, count_, next_, forward_);
	return res;
}

int	ListKeys(ClientObj client_, 
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

int	ListKeyValues(ClientObj client_, 
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

int Set(ClientObj client_, const std::string& key_, const std::string& value_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	ByteString			value(value_.length(), value_.length(), value_.c_str());

	return client->Set(key, value);
}

int TestAndSet(ClientObj client_, 
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

int64_t Add(ClientObj client_, const std::string& key_, int64_t num_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	int64_t				result;
	
	client->Add(key, num_, result);

	return result;
}

int Delete(ClientObj client_, const std::string& key_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	
	return client->Delete(key); 
}

int Remove(ClientObj client_, const std::string& key_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			key(key_.length(), key_.length(), key_.c_str());
	
	return client->Remove(key); 
}

int	Rename(ClientObj client_, const std::string& from_, const std::string& to_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			from(from_.length(), from_.length(), from_.c_str());
	ByteString			to(to_.length(), to_.length(), to_.c_str());
	
	return client->Rename(from, to); 
}

int Prune(ClientObj client_, const std::string& prefix_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	ByteString			prefix(prefix_.length(), prefix_.length(), prefix_.c_str());
	
	return client->Prune(prefix);
}

int Begin(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->Begin();
}

int Submit(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->Submit();
}

int Cancel(ClientObj client_)
{
	Keyspace::Client*	client = (Keyspace::Client *) client_;
	
	return client->Cancel();
}


