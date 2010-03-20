#ifndef CATCHUPCONN_H
#define CATCHUPCONN_H

#include "Framework/Transport/TCPConn.h"
#include "Framework/Database/Transaction.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "Framework/Paxos/PaxosConsts.h"
#include "CatchupMsg.h"

class CatchupServer;

class CatchupWriter : public TCPConn<>
{
typedef ByteArray<KEYSPACE_KEY_SIZE> KeyBuffer;
typedef ByteArray<RLOG_SIZE> ValBuffer;

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
	CatchupMsg		msg;
	uint64_t		paxosID;
	KeyBuffer		key;
	ValBuffer		value;
	CatchupServer*	server;
	ByteArray<32>	prefix;
	ByteArray<MAX_TCP_MESSAGE_SIZE> msgData;

};

#endif
