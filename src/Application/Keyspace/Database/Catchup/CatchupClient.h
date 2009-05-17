#ifndef CATCHUPCLIENT_H
#define CATCHUPCLIENT_H

#include "Framework/Transport/TCPConn.h"
#include "Framework/Database/Table.h"
#include "Framework/Database/Transaction.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "CatchupMsg.h"

#define CATCHUP_CONNECT_TIMEOUT	2000

class ReplicatedKeyspaceDB;

class CatchupClient : public TCPConn<>
{
public:
	void						Init(ReplicatedKeyspaceDB* keyspaceDB_, Table* table_);

	void						Start(unsigned nodeID);
	void						OnRead();
	void						OnClose();
	virtual void				OnConnect();
	virtual void				OnConnectTimeout();

private:
	void						ProcessMsg();
	void						OnKeyValue();
	void						OnCommit();

	Table*						table;
	CatchupMsg					msg;
	uint64_t					paxosID;
	Transaction					transaction;
	ReplicatedKeyspaceDB*		keyspaceDB;
};

#endif
