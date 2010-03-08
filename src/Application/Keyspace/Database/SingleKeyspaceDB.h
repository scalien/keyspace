#ifndef SINGLEKEYSPACEDB_H
#define SINGLEKEYSPACEDB_H

#include "Framework/Database/Database.h"
#include "Framework/Database/Transaction.h"
#include "KeyspaceDB.h"

class SingleKeyspaceDB : public KeyspaceDB
{
typedef ByteArray<KEYSPACE_VAL_META_SIZE> Buffer;
public:
	bool				Init();
	void				Shutdown();
	bool				Add(KeyspaceOp* op) ;
	bool				Submit();
	unsigned			GetNodeID() ;
	bool				IsMasterKnown();
	int					GetMaster();
	bool				IsMaster();
	bool				IsReplicated() { return false; }
	void				SetProtocolServer(ProtocolServer*) {}
	void				Stop() {}
	void				Continue() {}

private:
	bool				writePaxosID;
	Buffer				data;
	Table*				table;
	Transaction			transaction;
	List<KeyspaceOp*>	ops;
};

#endif
