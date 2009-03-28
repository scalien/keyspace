#ifndef CATCHUPCONN_H
#define CATCHUPCONN_H

#include "Framework/Transport/TCPConn.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "CatchupServer.h"
#include "CatchupMsg.h"

class CatchupConn : public TCPConn<BUF_SIZE>
{
public:
	CatchupConn(CatchupServer* server_);

	void				OnRead();
	void				OnWrite();
	
	virtual void		OnClose();
	
	void				WriteNext();
	
private:
	Buffer				writeBuffer;
	CatchupServer*		server;
	Cursor				cursor;
	Transaction			transaction;
	CatchupMsg			msg;
	ulong64				startPaxosID;
};

#endif
