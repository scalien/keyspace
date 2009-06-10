#ifndef KEYSPACECLIENT_H
#define KEYSPACECLIENT_H

#include "System/IO/Endpoint.h"
#include "System/IO/Socket.h"
#include "System/Buffer.h"
#include "System/Containers/List.h"
#include "System/Stopwatch.h"
#include "Framework/Transport/MessageConn.h"

#define KEYSPACE_OK			0
#define KEYSPACE_NOTMASTER	-1
#define KEYSPACE_FAILED		-2
#define KEYSPACE_ERROR		-3

namespace Keyspace
{

class Client;
class Command;

class Response
{
public:
	DynArray<128>	key;
	DynArray<128>	value;
	char			status;
	uint64_t		id;
	
	bool			Read(const ByteString& data);
private:
	char*			pos;
	char			separator;
	ByteString		data;
	ByteString		msg;
	
	bool			CheckOverflow();
	bool			ReadUint64(uint64_t& num);
	bool			ReadChar(char& c);
	bool			ReadSeparator();
	bool			ReadMessage(ByteString& bs);
	bool			ReadData(ByteString& bs, uint64_t length);

	bool			ValidateLength();
};

class ClientConn : public MessageConn<>
{
public:
	ClientConn(Client &client, int nodeID, const Endpoint &endpoint_);

	// MessageConn interface
	virtual void	OnMessageRead(const ByteString& msg);
	virtual void	OnWrite();
	virtual void	OnClose();
	virtual void	OnConnect();
	virtual void	OnConnectTimeout();

	Endpoint&		GetEndpoint();
	void			Send(Command &cmd);
	bool			ReadMessage(ByteString &msg);
	bool			ProcessResponse(Response* msg);
	void			GetMaster();
	void			DeleteCommands();

private:
	friend class Client;

	Client&			client;
	Endpoint		endpoint;
	bool			getMasterPending;
	int				nodeID;
	uint64_t		disconnectTime;
	uint64_t		getMasterTime;
};

typedef List<Response*> ResponseList;

class Result
{
public:
	void				Close();
	Result*				Next(int &status);

	const ByteString&	Key();
	const ByteString&	Value();
	int					Status();

private:
	friend class ClientConn;
	friend class Client;
	
	ResponseList		responses;
	ByteString			empty;
	
	void				AppendResponse(Response* resp);
};

class Command
{
public:
	Command();

	char				type;
	DynArray<128>		msg;
	int					nodeID;
	int					status;
	uint64_t			cmdID;
	bool				submit;
};

typedef List<Command*> CommandList;

class Client
{
public:
	~Client();
	
	int				Init(int nodec, const char* nodev[], uint64_t timeout);
	
	// connection state related commands
	int				GetMaster();
	int				GetState(int node);
	void			DistributeDirty(bool dd);
	
	// simple get commands with preallocated value
	int				Get(const ByteString &key, ByteString &value, bool dirty = false);
	int				DirtyGet(const ByteString &key, ByteString &value);

	// commands that return a Result
	int				Get(const ByteString &key, bool dirty = false);
	int				DirtyGet(const ByteString &key);
	int				List(const ByteString &prefix, uint64_t count = 0, bool dirty = false);
	int				DirtyList(const ByteString &prefix, uint64_t count = 0);
	int				ListP(const ByteString &prefix, uint64_t count = 0, bool dirty = false);
	int				DirtyListP(const ByteString &prefix, uint64_t count = 0);

	Result*			GetResult(int &status);

	// write commands
	int				Set(const ByteString &key, const ByteString &value, bool sumbit = true);
	int				TestAndSet(const ByteString &key, const ByteString &test, const ByteString &value, 
							   bool submit = true);
	int				Add(const ByteString &key, int64_t num, int64_t &result, bool submit = true);
	int				Delete(const ByteString &key, bool submit = true);

	// grouping write commands
	int				Begin();
	int				Submit();

private:
	friend class ClientConn;
	
	void			StateFunc();
	void			EventLoop();
	bool			IsDone();
	uint64_t		GetNextID();
	Command*		CreateCommand(char cmd, bool submit, int msgc, ByteString *msgv);
	void			SendCommand(ClientConn* conn, CommandList& commands);
	void			DeleteCommands(CommandList& commands);
	void			SetMaster(int master);
	
	
	CommandList		safeCommands;
	CommandList		dirtyCommands;
	CommandList		sentCommands;
	ClientConn		**conns;
	int				numConns;
	int				numFinished;
	int				master;
	uint64_t		timeout;
	uint64_t		reconnectTimeout;
	uint64_t		masterTime;
	uint64_t		cmdID;
	Result			result;
	bool			distributeDirty;
};

}; // namespace

#endif
