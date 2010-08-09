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
	
	int				Init(int nodec, const char* nodev[]);
	void			Shutdown();

	void			SetGlobalTimeout(uint64_t timeout);
	void			SetMasterTimeout(uint64_t timeout);
	uint64_t		GetGlobalTimeout();
	uint64_t		GetMasterTimeout();
	
	// connection state related commands
	int				GetMaster();
	void			DistributeDirty(bool dd);
	
	Result*			GetResult();

	int				TransportStatus();
	int				ConnectivityStatus();
	int				TimeoutStatus();
	int				CommandStatus();
				
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
	int				Get(const ByteString &key, bool dirty = false);
	int				DirtyGet(const ByteString &key);

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
						const ByteString &value);
	int				TestAndSet(const ByteString &key,
							   const ByteString &test,
							   const ByteString &value);
	int				Add(const ByteString &key, int64_t num, int64_t &result);
	int				Delete(const ByteString &key, bool remove = false);
	int				Remove(const ByteString &key);
	int				Rename(const ByteString &from, const ByteString &to);
	int				Prune(const ByteString &prefix);
	int				SetExpiry(const ByteString &key, uint64_t expiryTime);
	int				RemoveExpiry(const ByteString &key);

	// grouping write commands
	int				Begin();
	int				Submit();
	int				Cancel();
	bool			IsBatched();

private:
	friend class ClientConn;
	typedef MFunc<Client> Func;
	
	void			StateFunc();
	void			EventLoop();
	bool			IsDone();
	uint64_t		NextMasterCommandID();
	uint64_t		NextCommandID();
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
	bool			IsSafe();
	
	CommandList		safeCommands;
	CommandList		dirtyCommands;
	ClientConn**	conns;
	int				numConns;
	int				numFinished;
	int				master;
	uint64_t		masterTime;
	uint64_t		masterCmdID;
	uint64_t		cmdID;
	Result*			result;
	bool			masterQuery;
	bool			distributeDirty;
	int				currentConn;
	int				connectivityStatus;
	int				timeoutStatus;

	Func			onGlobalTimeout;
	CdownTimer		globalTimeout;
	Func			onMasterTimeout;
	CdownTimer		masterTimeout;
};

}; // namespace

#endif
