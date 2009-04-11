#ifndef KEYSPACECONN_H
#define KEYSPACECONN_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/TCPConn.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Database/KeyspaceClient.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientMsg.h"

class KeyspaceServer;

#define RESP_SIZE	KEYSPACE_VAL_SIZE + 1*KB

class KeyspaceConn : public TCPConn<>, public KeyspaceClient
{
public:
	KeyspaceConn();
	
	void				Init(KeyspaceDB* kdb_, KeyspaceServer* server_);
	void				OnComplete(KeyspaceOp* op, bool status, bool final);

private:
	// TCPConn interface
	virtual void		OnRead();
	virtual void		OnClose();
	virtual void		OnWrite();

	void				Write(ByteString &bs);
	void				ProcessMsg();
	void				AppendOps();
	
	ByteArray<RESP_SIZE>response;
	KeyspaceServer*		server;
	KeyspaceClientReq	msg;
	bool				closeAfterSend;
};

#undef RESP_SIZE

#endif
