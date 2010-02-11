#ifndef KEYSPACECONN_H
#define KEYSPACECONN_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/MessageConn.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Database/KeyspaceService.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientReq.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientResp.h"

#define KEYSPACE_CONN_TIMEOUT 3000

class KeyspaceServer;

class KeyspaceConn : public MessageConn<KEYSPACE_BUF_SIZE>,
public KeyspaceService
{
friend class KeyspaceServer;
typedef MFunc<KeyspaceConn>				Func;
typedef ByteArray<KEYSPACE_BUF_SIZE>	Buffer;
public:
	KeyspaceConn();
	
	void				Init(KeyspaceDB* kdb_, KeyspaceServer* server_);

	// KeyspaceService interface
	virtual void		OnComplete(KeyspaceOp* op, bool final);
	virtual bool		IsAborted();

private:
	// TCPConn interface
	virtual void		OnClose();
	virtual void		OnWrite();
	virtual void		OnMessageRead(const ByteString& message);

	void				Write(ByteString &bs);
	void				ProcessMsg();
	void				AppendOps();

	Buffer				data;
	KeyspaceServer*		server;
	KeyspaceClientReq	req;
	KeyspaceClientResp	resp;
	bool				closeAfterSend;
	char				endpointString[ENDPOINT_STRING_SIZE];
	unsigned			bytesRead;
};

#endif
