#ifndef KEYSPACEDB_H
#define KEYSPACEDB_H

#include "System/Containers/List.h"
#include "System/Buffer.h"
#include "System/Events/Callable.h"
#include "Framework/Database/Database.h"
#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedDB/ReplicatedDB.h"
#include "Framework/AsyncDatabase/MultiDatabaseOp.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Catchup/CatchupServer.h"
#include "Catchup/CatchupClient.h"
#include "KeyspaceConsts.h"
#include "KeyspaceMsg.h"

class KeyspaceDB : ReplicatedDB
{
public:
	KeyspaceDB();
	
	bool					Init();
	bool					Add(KeyspaceOp* op, bool submit = true);
	bool					Submit();
	unsigned				GetNodeID();
	bool					IsMasterKnown();
	int						GetMaster();
	bool					IsMaster();
	void					OnCatchupComplete();	// called by CatchupClient
	void					OnCatchupFailed();		// called by CatchupClient
	
// ReplicatedDB interface:
	virtual void			OnAppend(Transaction* transaction, uint64_t paxosID,
									 ByteString value, bool ownAppend);
	virtual void			OnMasterLease(unsigned nodeID);
	virtual void			OnMasterLeaseExpired();
	virtual void			OnDoCatchup(unsigned nodeID);
	
private:
	bool					AddWithoutReplicatedLog(KeyspaceOp* op, bool submit);
	void					Execute(Transaction* transaction, bool ownAppend);
	void					Append();
	void					FailKeyspaceOps();
	
	bool					catchingUp;
	bool					writePaxosID;  // single node case
	List<KeyspaceOp*>		ops;
	Table*					table;
	KeyspaceMsg				msg;
	ByteArray<KEYSPACE_BUF_SIZE>pvalue;
	ByteArray<KEYSPACE_VAL_SIZE>data;
	CatchupServer			catchupServer;
	CatchupClient			catchupClient;
};

#endif
