#ifndef CATCHUPCONN_H
#define CATCHUPCONN_H

#include "Framework/Transport/TCPConn.h"
#include "Framework/Database/Transaction.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "CatchupMsg.h"

class CatchupServer;

class CatchupWriter : public TCPConn<>
{
typedef ByteArray<KEYSPACE_KEY_SIZE> KeyBuffer;
typedef ByteArray<KEYSPACE_VAL_SIZE> ValBuffer;

public:
	CatchupWriter();

	void			Init(CatchupServer* server);
	
	void			OnRead();
	void			OnWrite();	
	virtual void	OnClose();
	
private:
	void			WriteNext();
	Buffer			writeBuffer;
	Table*			table;
	Cursor			cursor;
	Transaction		transaction;
	CatchupMsg		msg;
	uint64_t		paxosID;
	KeyBuffer		key;
	ValBuffer		value;
	CatchupServer*	server;
};

#endif
