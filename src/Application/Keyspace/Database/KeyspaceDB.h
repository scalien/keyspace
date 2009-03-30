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
#include "KeyspaceClient.h"

class KeyspaceDB : ReplicatedDB
{
public:
	KeyspaceDB();
	
	bool					Init();
	bool					Add(KeyspaceOp& op);	// the interface used by KeyspaceClient
	unsigned				GetNodeID();
	void					OnCatchupComplete();	// called by CatchupClient
	void					OnCatchupFailed();		// called by CatchupClient
	
// ReplicatedDB interface:
	virtual void			OnAppend(Transaction* transaction, ulong64 paxosID,
									 ByteString value, bool ownAppend);
	virtual void			OnMasterLease(unsigned nodeID);
	virtual void			OnMasterLeaseExpired();
	virtual void			OnDoCatchup(unsigned nodeID);
	
private:
	void					Execute(Transaction* transaction, bool ownAppend);
	void					Append();
	
	bool					catchingUp;
	List<KeyspaceOp>		ops;
	Table*					table;
	KeyspaceMsg				msg;
	ByteArray<VALUE_SIZE>	data;
	CatchupServer			catchupServer;
	CatchupClient			catchupClient;
};

#endif
