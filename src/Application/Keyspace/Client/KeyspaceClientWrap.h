#ifndef KEYSPACE_CLIENT_WRAP_H
#define KEYSPACE_CLIENT_WRAP_H

// SWIG compatible wrapper interface for generating client wrappers

#include <string>

typedef void * ClientObj;
typedef void * ResultObj;

struct NodeParams
{
	NodeParams(int nodec_)
	{
		num = 0;
		nodec = nodec_;
		nodes = new char*[nodec];
		for (int i = 0; i < nodec; i++)
			nodes[i] = NULL;
	}
	
	~NodeParams()
	{
		Close();
	}
	
	void Close()
	{
		for (int i = 0; i < num; i++)
			free(nodes[i]);
		delete[] nodes;
		nodes = NULL;
		num = 0;
	}
	
	void AddNode(const std::string& node)
	{
		if (num > nodec)
			return;
		nodes[num++] = strdup(node.c_str());
	}

	int				nodec;
	char**			nodes;
	int				num;
};

void		ResultBegin(ResultObj result);
void		ResultNext(ResultObj result);
bool		ResultIsEnd(ResultObj result);
void		ResultClose(ResultObj result);
std::string	ResultKey(ResultObj result);
std::string	ResultValue(ResultObj result);
int			ResultTransportStatus(ResultObj result);
int			ResultConnectivityStatus(ResultObj result);
int			ResultTimeoutStatus(ResultObj result);
int			ResultCommandStatus(ResultObj result);

ClientObj	Create();
int			Init(ClientObj client, const NodeParams &params);
ResultObj	GetResult(ClientObj);

int			Get(ClientObj client, const std::string& key);
int			DirtyGet(ClientObj client, const std::string& key);
int			Count(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			DirtyCount(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);

int			ListKeys(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			DirtyListKeys(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);

int			ListKeyValues(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			DirtyListKeyValues(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
					 
int			Set(ClientObj client, const std::string& key, const std::string& value);
int			TestAndSet(ClientObj client, 
					   const std::string& key,
					   const std::string& test,
					   const std::string& value);
int			Add(ClientObj client, const std::string& key, int64_t num);
int			Delete(ClientObj client, const std::string& key);
int			Remove(ClientObj client, const std::string& key);
int			Rename(ClientObj client, const std::string& from, const std::string& to);
int			Prune(ClientObj client, const std::string& prefix);

// grouping write commands
int			Begin(ClientObj client);
int			Submit(ClientObj client);
int			Cancel(ClientObj client);
bool		IsBatched(ClientObj client);

#endif
