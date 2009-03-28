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
#include "KeyspaceConsts.h"
#include "KeyspaceMsg.h"
#include "KeyspaceClient.h"

class KeyspaceOp_Alloc : public KeyspaceOp
{
public:
	KeyspaceOp_Alloc() { pkey = NULL; pvalue = NULL; ptest = NULL; }
	
	char*	pkey;
	char*	pvalue;
	char*	ptest;
	
	void	Alloc(KeyspaceOp& op);
	void	Free();
};

class KeyspaceDB : ReplicatedDB
{
public:
	KeyspaceDB();
	
	bool					Init(ReplicatedLog* replicatedLog_);
	bool					Add(KeyspaceOp& op);	// the interface used by KeyspaceClient
	unsigned				GetNodeID();
	
// ReplicatedDB interface:
	virtual void			OnAppend(Transaction* transaction, ulong64 paxosID,
								ByteString value, bool ownAppend);
	virtual void			OnMasterLease(unsigned nodeID);
	virtual void			OnMasterLeaseExpired();
	virtual void			OnDoCatchup();
	
private:
	void					Execute(Transaction* transaction, bool ownAppend);
	void					Append();
	
	ReplicatedLog*			replicatedLog;
	List<KeyspaceOp>		queuedOps;
	Table*					table;
	KeyspaceMsg				msg;
	List<KeyspaceOp_Alloc>	ops;
	ByteArray<VALUE_SIZE>	data;
	bool					catchingUp;
};

#endif
