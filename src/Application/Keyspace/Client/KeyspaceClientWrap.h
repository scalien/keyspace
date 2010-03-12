#ifndef KEYSPACE_CLIENT_WRAP_H
#define KEYSPACE_CLIENT_WRAP_H

// SWIG compatible wrapper interface for generating client wrappers

#include <string>
#include <vector>

typedef void * ClientObj;
typedef void * ResultObj;

struct NodeParams
{
	NodeParams(int nodec_)
	{
		i = 0;
		nodec = nodec_;
		nodes = new const char*[nodec];
	}
	
	~NodeParams()
	{
		Close();
	}
	
	void Close()
	{
		delete[] nodes;
		nodes = NULL;
		i = 0;
	}
	
	void AddNode(const char* node)
	{
		if (i > nodec)
			return;
		nodes[i++] = node;
	}

	int				nodec;
	const char**	nodes;
	int				i;
};

void		ResultBegin(ResultObj result);
void		ResultNext(ResultObj result);
bool		ResultIsEnd(ResultObj result);
void		ResultClose(ResultObj result);
std::string	ResultKey(ResultObj result);
std::string	ResultValue(ResultObj result);

ClientObj	Create();
int			Init(ClientObj client, const NodeParams &params);
ResultObj	GetResult(ClientObj);

int			Get(ClientObj client, const std::string& key);
//ReturnValue DirtyGet(ClientObj client, const std::string& key);
uint64_t	Count(ClientObj client, 
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

int			ListKeyValues(ClientObj client, 
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
int64_t		Add(ClientObj client, const std::string& key, int64_t num);
int			Delete(ClientObj client, const std::string& key);
int			Remove(ClientObj client, const std::string& key);
int			Rename(ClientObj client, const std::string& from, const std::string& to);
int			Prune(ClientObj client, const std::string& prefix);

// grouping write commands
int			Begin(ClientObj client);
int			Submit(ClientObj client);
int			Cancel(ClientObj client);

#endif
