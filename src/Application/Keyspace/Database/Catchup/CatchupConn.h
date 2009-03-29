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

	void				Init();
	
	void				OnRead();
	void				OnWrite();	
	virtual void		OnClose();
	
private:
	void				WriteNext();
	Buffer				writeBuffer;
	CatchupServer*		server;
	Cursor				cursor;
	Transaction			transaction;
	CatchupMsg			msg;
	ulong64				paxosID;
	ByteArray<KEY_SIZE>	key;
	ByteArray<VAL_SIZE>	value;
};

#endif
