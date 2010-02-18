#ifndef KEYSPACE_CLIENT_CONN_H
#define KEYSPACE_CLIENT_CONN_H

#include "System/Containers/List.h"
#include "Framework/Transport/MessageConn.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"

#define KEYSPACE_MOD_GETMASTER		0
#define KEYSPACE_MOD_COMMAND		1

namespace Keyspace
{

class Client;
class Response;
class Command;
typedef List<Command*> CommandList;

class ClientConn : public MessageConn<KEYSPACE_BUF_SIZE>
{
typedef MFunc<ClientConn> Func;

public:
	ClientConn(Client &client, int nodeID, const Endpoint &endpoint_);

	void			Connect();
	void			Send(Command &cmd);
	void			SendSubmit();
	void			SendGetMaster();

	// MessageConn interface
	virtual void	OnMessageRead(const ByteString& msg);
	virtual void	OnWrite();
	virtual void	OnClose();
	virtual void	OnConnect();
	virtual void	OnConnectTimeout();

	void			OnReadTimeout();
	void			OnGetMaster();
	
	Endpoint&		GetEndpoint();
	bool			ReadMessage(ByteString &msg);
	bool			ProcessResponse(Response* msg);
	bool			ProcessGetMaster(Response* resp);
	bool			ProcessCommand(Response* resp);
	void			GetMaster();
	void			DeleteCommands();

private:
	friend class Client;

	bool			submit;
	Client&			client;
	Endpoint		endpoint;
	int				nodeID;
	uint64_t		getMasterTime;
	Func			onGetMasterTimeout;
	CdownTimer		getMasterTimeout;
	CommandList		getMasterCommands;
};

}; // namespace

#endif
