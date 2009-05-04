#ifndef KEYSPACECONN_H
#define KEYSPACECONN_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/TCPConn.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Database/KeyspaceService.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientMsg.h"

#define KEYSPACE_CONN_TIMEOUT 3000

class KeyspaceServer;

class KeyspaceConn : public TCPConn<>, public KeyspaceService
{
public:
	KeyspaceConn();
	
	void				Init(KeyspaceDB* kdb_, KeyspaceServer* server_);
	void				OnComplete(KeyspaceOp* op, bool status, bool final);
	void				OnConnectionTimeout();

private:
	// TCPConn interface
	virtual void		OnRead();
	virtual void		OnClose();
	virtual void		OnWrite();

	void				Write(ByteString &bs);
	void				ProcessMsg();
	void				AppendOps();

	MFunc<KeyspaceConn>	onConnectionTimeout;
	CdownTimer			connectionTimeout;
	ByteArray<KEYSPACE_BUF_SIZE> data;
	KeyspaceServer*		server;
	KeyspaceClientReq	req;
	KeyspaceClientResp	resp;
	bool				closeAfterSend;
};

#endif
