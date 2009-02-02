#ifndef REPLICATEDLOG_H
#define REPLICATEDLOG_H

#include "System/Containers/List.h"
#include "System/Events/Callable.h"
#include "Framework/Paxos/Paxos.h"
#include "Framework/Paxos/PaxosMsg.h"
#include "Framework/ReplicatedDB/ReplicatedDB.h"
#include "Framework/MasterLease/MasterLease.h"
#include "LogCache.h"

#define CATCHUP_TIMEOUT	5000

class ReplicatedLog : private Paxos
{
public:
	ReplicatedLog();

	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_, bool multiPaxos);
	
	bool				Append(ByteString value);
	bool				Cancel();
	
	void				SetReplicatedDB(ReplicatedDB* replicatedDB);
	
	Entry*				LastEntry();
	
	void				SetMaster(bool master);	// multi paxos
	bool				IsMaster();				// multi paxos
	
	int					NodeID();
	
	bool				ReadConfig(char* filename);

	bool				Stop();
	bool				Continue();

protected:
	bool				appending;
	ByteString			value;
	
	LogCache			logCache;
	
	bool				PersistState(Transaction* transaction); // TODO: call after OnAppend()
	
	virtual void		OnPrepareRequest();
	virtual void		OnPrepareResponse();
	virtual void		OnProposeRequest();
	virtual void		OnProposeResponse();
	virtual void		OnLearnChosen();
	
	void				OnCatchupTimeout();
	TimerM<ReplicatedLog>catchupTimeout;

	MasterLease			masterLease;
	
	ReplicatedDB*		replicatedDB;
};

#endif