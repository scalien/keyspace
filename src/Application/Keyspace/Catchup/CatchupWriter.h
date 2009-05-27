#ifndef CATCHUPCONN_H
#define CATCHUPCONN_H

#include "Framework/Transport/TCPConn.h"
#include "Framework/Paxos/PaxosConsts.h"
#include "Framework/Database/Transaction.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "CatchupMsg.h"

class CatchupServer;

class CatchupWriter : public TCPConn<>
{
public:
	CatchupWriter();

	void							Init();
	
	void							OnRead();
	void							OnWrite();	
	virtual void					OnClose();
	
private:
	void							WriteNext();
	Buffer							writeBuffer;
	Table*							table;
	Cursor							cursor;
	Transaction						transaction;
	CatchupMsg						msg;
	uint64_t						paxosID;
	ByteArray<KEYSPACE_KEY_SIZE>	key;
	ByteArray<KEYSPACE_VAL_SIZE>	value;
};

#endif
