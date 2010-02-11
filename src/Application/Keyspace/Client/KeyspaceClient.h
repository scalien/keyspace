#ifndef KEYSPACECLIENT_H
#define KEYSPACECLIENT_H

#include "System/IO/Endpoint.h"
#include "System/IO/Socket.h"
#include "System/Buffer.h"
#include "System/Containers/List.h"
#include "System/Stopwatch.h"
#include "System/Events/Callable.h"
#include "System/Events/Timer.h"
#include "KeyspaceClientConsts.h"
#include "KeyspaceResult.h"

namespace Keyspace
{

class Client
{
public:
	Client();
	~Client();
	
	int				Init(int nodec, const char* nodev[], uint64_t timeout);
	uint64_t		SetTimeout(uint64_t timeout);
	uint64_t		GetTimeout();
	
	// connection state related commands
	int				GetMaster();
	int				GetState(int node);
	void			DistributeDirty(bool dd);
	void			SetAutoFailover(bool fo);
	
	Result*			GetResult();
	int				TransportStatus();
	int				ConnectivityStatus();
	int				TimeoutStatus();
				
	// simple get commands with preallocated value
	int				Get(const ByteString &key, ByteString &value,
						bool dirty = false);
	int				DirtyGet(const ByteString &key, ByteString &value);

	// commands that return a number as res
	int				Count(uint64_t &res, const ByteString &prefix,
						  const ByteString &startKey,
						  uint64_t count = 0,
						  bool next = false, bool forward = true);
	int				DirtyCount(uint64_t &res, const ByteString &prefix,
							   const ByteString &startKey,
							   uint64_t count = 0,
							    bool next = false, bool forward = true);

	// commands that return a Result
	int				Get(const ByteString &key, bool dirty = false,
						bool submit = true);
	int				DirtyGet(const ByteString &key, bool submit = true);

	int				ListKeys(const ByteString &prefix,
							 const ByteString &startKey,
							 uint64_t count = 0,
							 bool next = false, bool forward = true);
	int				DirtyListKeys(const ByteString &prefix,
								  const ByteString &startKey,
								  uint64_t count = 0,
								  bool next = false, bool forward = true);
	int				ListKeyValues(const ByteString &prefix,
								  const ByteString &startKey,
								  uint64_t count = 0,
								  bool next = false, bool forward = true);
	int				DirtyListKeyValues(const ByteString &prefix,
									   const ByteString &startKey,
									   uint64_t count = 0,
									   bool next = false, bool forward = true);

	// write commands
	int				Set(const ByteString &key,
						const ByteString &value,
						bool sumbit = true);
	int				TestAndSet(const ByteString &key,
							   const ByteString &test,
							   const ByteString &value, 
							   bool submit = true);
	int				Add(const ByteString &key, int64_t num,
						int64_t &result, bool submit = true);
	int				Delete(const ByteString &key, bool submit = true,
						   bool remove = false);
	int				Remove(const ByteString &key, bool submit = true);
	int				Rename(const ByteString &from, const ByteString &to,
						   bool submit = true);
	int				Prune(const ByteString &prefix, bool submit = true);

	// grouping write commands
	int				Begin();
	int				Submit();
	int				Cancel();

private:
	friend class ClientConn;
	typedef MFunc<Client> Func;
	
	void			StateFunc();
	void			EventLoop();
	bool			IsDone();
	uint64_t		GetNextID();
	Command*		CreateCommand(char cmd, int msgc, ByteString *msgv);
	void			SendCommand(ClientConn* conn, CommandList& commands);
	void			SendDirtyCommands();
	void			SetMaster(int master, int node);
	int				Count(uint64_t &res, const ByteString &prefix,
						  const ByteString &startKey,
						  uint64_t count, bool next,
						  bool forward, bool dirty);
	int				ListKeyValues(const ByteString &prefix,
								  const ByteString &startKey,
								  uint64_t count, bool next,
								  bool forward, bool dirty, bool values);
	void			OnGlobalTimeout();
	void			OnMasterTimeout();
	
	CommandList		safeCommands;
	CommandList		dirtyCommands;
	ClientConn**	conns;
	int				numConns;
	int				numFinished;
	int				master;
	uint64_t		timeout;
	uint64_t		masterTime;
	uint64_t		cmdID;
	Result*			result;
	bool			masterQuery;
	bool			distributeDirty;
	bool			autoFailover;
	int				currentConn;
	int				connectivityStatus;
	int				timeoutStatus;

	CdownTimer		globalTimeout;
	Func			onGlobalTimeout;
	CdownTimer		masterTimeout;
	Func			onMasterTimeout;
};

}; // namespace

#endif
