#ifndef SINGLEKEYSPACEDB_H
#define SINGLEKEYSPACEDB_H

#include "Framework/Database/Database.h"
#include "Framework/Database/Transaction.h"
#include "KeyspaceDB.h"

class SingleKeyspaceDB : public KeyspaceDB
{
public:
	bool			Init();
	bool			Add(KeyspaceOp* op) ;
	bool			Submit();
	unsigned		GetNodeID() ;
	bool			IsMasterKnown();
	int				GetMaster();
	bool			IsMaster();

private:
	bool			writePaxosID;
	ByteArray<KEYSPACE_VAL_SIZE>data;
	Table*			table;
	Transaction		transaction;
};

#endif
