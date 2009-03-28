#ifndef CATCHUPCLIENT_H
#define CATCHUPCLIENT_H

#include "Framework/Transport/TCPConn.h"
#include "Framework/Database/Table.h"
#include "Framework/Database/Transaction.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "CatchupMsg.h"

class KeyspaceDB;

class CatchupClient : public TCPConn<>
{
public:
	void						Init(KeyspaceDB* keyspaceDB_, Table* table_);

	void						Start(unsigned nodeID);
	void						OnRead();
	void						OnClose();

private:
	void						ProcessMsg();
	void						OnKeyValue();
	void						OnCommit();

	Table*						table;
	CatchupMsg					msg;
	ulong64						paxosID;
	Transaction					transaction;
	KeyspaceDB*					keyspaceDB;
};

#endif
