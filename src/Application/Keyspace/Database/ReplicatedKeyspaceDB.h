#ifndef REPLICATEDKEYSPACEDB_H
#define REPLICATEDKEYSPACEDB_H

#include "System/Buffer.h"
#include "System/Containers/List.h"
#include "System/Events/Callable.h"
#include "Framework/Database/Database.h"
#include "Framework/Database/Transaction.h"
#include "Framework/ReplicatedLog/ReplicatedDB.h"
#include "Framework/AsyncDatabase/MultiDatabaseOp.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Application/Keyspace/Catchup/CatchupServer.h"
#include "Application/Keyspace/Catchup/CatchupReader.h"
#include "KeyspaceMsg.h"
#include "KeyspaceDB.h"

class ReplicatedKeyspaceDB : public ReplicatedDB, public KeyspaceDB
{
typedef ByteArray<PAXOS_SIZE>				PaxosBuffer;
typedef ByteArray<KEYSPACE_KEY_META_SIZE>	KeyBuffer;
typedef ByteArray<KEYSPACE_VAL_META_SIZE>	ValBuffer;
typedef MFunc<ReplicatedKeyspaceDB>			Func;
typedef List<KeyspaceOp*>					OpList;
typedef List<ProtocolServer*>				ServerList;

public:
	ReplicatedKeyspaceDB();
	~ReplicatedKeyspaceDB();
	
	bool			Init();
	void			Shutdown();
	bool			Add(KeyspaceOp* op);
	bool			Submit();
	unsigned		GetNodeID();
	bool			IsMasterKnown();
	int				GetMaster();
	bool			IsMaster();
	bool			IsReplicated() { return true; }
	void			SetProtocolServer(ProtocolServer* pserver);
	
	void			OnCatchupComplete();	// called by CatchupClient
	void			OnCatchupFailed();		// called by CatchupClient
	void			OnExpiryTimer();
	
// ReplicatedDB interface:
	virtual void	OnAppend(Transaction* transaction, uint64_t paxosID,
							 ByteString value, bool ownAppend);
	virtual void	OnMasterLease();
	virtual void	OnMasterLeaseExpired();
	virtual void	OnDoCatchup(unsigned nodeID);
	
	void			AsyncOnAppend();
	void			OnAppendComplete();

	bool			IsCatchingUp();
	
	bool			DeleteDB();
	
private:
	bool			AddWithoutReplicatedLog(KeyspaceOp* op);
	bool			Execute(Transaction* transaction,
							uint64_t paxosID, uint64_t commandID);
	void			Append();
	void			FailKeyspaceOps();
	void			InitExpiryTimer();
	uint64_t		GetExpiryTime(ByteString key);
	
	bool			asyncAppenderActive;
	bool			catchingUp;
	OpList			ops;
	Table*			table;
	KeyspaceMsg		msg;
	KeyspaceMsg		tmp;
	PaxosBuffer		pvalue;
	KeyBuffer		kdata;
	ValBuffer		rdata;
	ValBuffer		wdata;
	CatchupServer	catchupServer;
	CatchupReader	catchupClient;
	
	Transaction*	transaction;
	ByteBuffer		valueBuffer;
	ByteBuffer		tmpBuffer;
	bool			ownAppend;
	uint64_t		paxosID;
	Func			asyncOnAppend;
	Func			onAppendComplete;
	ThreadPool*		asyncAppender;
	unsigned		numOps;
	ServerList		pservers;
	unsigned		estimatedLength;
	bool			deleteDB;
	
	Func			onExpiryTimer;
	Timer			expiryTimer;
};

#endif
