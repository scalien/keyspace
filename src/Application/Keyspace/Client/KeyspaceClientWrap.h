#ifndef KEYSPACE_CLIENT_WRAP_H
#define KEYSPACE_CLIENT_WRAP_H

// SWIG compatible wrapper interface for generating client wrappers

#include <string>
#include "System/Platform.h"

typedef void * ClientObj;
typedef void * ResultObj;

// helper class for converting node array to Init argument
struct Keyspace_NodeParams
{
	Keyspace_NodeParams(int nodec_);
	~Keyspace_NodeParams();

	void Close();
	void AddNode(const std::string& node);

	int				nodec;
	char**			nodes;
	int				num;
};

void		Keyspace_ResultBegin(ResultObj result);
void		Keyspace_ResultNext(ResultObj result);
bool		Keyspace_ResultIsEnd(ResultObj result);
void		Keyspace_ResultClose(ResultObj result);
std::string	Keyspace_ResultKey(ResultObj result);
std::string	Keyspace_ResultValue(ResultObj result);
int			Keyspace_ResultTransportStatus(ResultObj result);
int			Keyspace_ResultConnectivityStatus(ResultObj result);
int			Keyspace_ResultTimeoutStatus(ResultObj result);
int			Keyspace_ResultCommandStatus(ResultObj result);

ClientObj	Keyspace_Create();
int			Keyspace_Init(ClientObj client, const Keyspace_NodeParams &params);
void		Keyspace_Destroy(ClientObj client);
ResultObj	Keyspace_GetResult(ClientObj);

void		Keyspace_SetGlobalTimeout(ClientObj client, uint64_t timeout);
void		Keyspace_SetMasterTimeout(ClientObj client, uint64_t timeout);
uint64_t	Keyspace_GetGlobalTimeout(ClientObj client);
uint64_t	Keyspace_GetMasterTimeout(ClientObj client);

// connection state related commands
int			Keyspace_GetMaster(ClientObj client);
void		Keyspace_DistributeDirty(ClientObj client, bool dd);

int			Keyspace_Get(ClientObj client, const std::string& key);
int			Keyspace_DirtyGet(ClientObj client, const std::string& key);

int			Keyspace_Count(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			Keyspace_CountStr(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 const std::string& count,
					 bool next,
					 bool forward);

int			Keyspace_DirtyCount(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			Keyspace_DirtyCountStr(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 const std::string& count,
					 bool next,
					 bool forward);

int			Keyspace_ListKeys(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			Keyspace_ListKeysStr(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 const std::string& count,
					 bool next,
					 bool forward);

int			Keyspace_DirtyListKeys(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			Keyspace_DirtyListKeysStr(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 const std::string& count,
					 bool next,
					 bool forward);

int			Keyspace_ListKeyValues(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			Keyspace_ListKeyValuesStr(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 const std::string& count,
					 bool next,
					 bool forward);

int			Keyspace_DirtyListKeyValues(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 uint64_t count,
					 bool next,
					 bool forward);
int			Keyspace_DirtyListKeyValuesStr(ClientObj client, 
					 const std::string& prefix,
					 const std::string& startKey,
					 const std::string& count,
					 bool next,
					 bool forward);
					 
int			Keyspace_Set(ClientObj client, const std::string& key, const std::string& value);
int			Keyspace_TestAndSet(ClientObj client, 
					   const std::string& key,
					   const std::string& test,
					   const std::string& value);
int			Keyspace_Add(ClientObj client, const std::string& key, int64_t num);
int			Keyspace_AddStr(ClientObj client, const std::string& key, const std::string& num);
int			Keyspace_Delete(ClientObj client, const std::string& key);
int			Keyspace_Remove(ClientObj client, const std::string& key);
int			Keyspace_Rename(ClientObj client, const std::string& from, const std::string& to);
int			Keyspace_Prune(ClientObj client, const std::string& prefix);

// grouping write commands
int			Keyspace_Begin(ClientObj client);
int			Keyspace_Submit(ClientObj client);
int			Keyspace_Cancel(ClientObj client);
bool		Keyspace_IsBatched(ClientObj client);

// debugging command
void		Keyspace_SetTrace(bool trace);

#endif
