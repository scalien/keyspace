#ifndef CATCHUPCLIENT_H
#define CATCHUPCLIENT_H

#include "Framework/Transport/TCPConn.h"
#include "Framework/Database/Table.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "CatchupMsg.h"

class CatchupClient : TCPConn<BUF_SIZE>
{
public:
	void				Init(Table* table_);

	void				OnRead();
	void				OnClose();

private:
	void				ProcessMsg();
	void				Execute(ulong64 paxosID, ByteString& dbCommand);
	
	void				OnKeyValue();
	void				OnDBCommand();
	void				OnCommit();
	void				OnRollback();

	Table*				table;
	CatchupMsg			msg;
	ulong64				paxosID;
};

#endif
