#ifndef SINGLEKEYSPACEDB_H
#define SINGLEKEYSPACEDB_H

#include "Framework/Database/Database.h"
#include "Framework/Database/Transaction.h"
#include "KeyspaceDB.h"

class SingleKeyspaceDB : public KeyspaceDB
{
typedef ByteArray<KEYSPACE_KEY_META_SIZE>	KBuffer;
typedef ByteArray<KEYSPACE_VAL_META_SIZE>	VBuffer;
typedef MFunc<SingleKeyspaceDB>				Func;
typedef List<KeyspaceOp*>					OpList;

public:
	SingleKeyspaceDB();
	
	bool				Init();
	void				InitExpiryTimer();
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
	void				OnExpiryTimer();
	
private:
	bool				writePaxosID;
	KBuffer				kdata;
	VBuffer				vdata;
	Table*				table;
	Transaction			transaction;
	Func				onExpiryTimer;
	Timer				expiryTimer;
	OpList              listOps;
    Func                onListWorkerTimeout;
    CdownTimer          listTimer;

    void                ExecuteListWorkers();
    void                ExecuteListWorker(KeyspaceOp** it);
    void                OnListWorkerTimeout();
};

#endif
