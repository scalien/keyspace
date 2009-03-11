#ifndef REPLICATEDLOG_H
#define REPLICATEDLOG_H

#include "System/Containers/List.h"
#include "System/Events/Callable.h"
#include "Framework/Paxos/PaxosProposer.h"
#include "Framework/Paxos/PaxosAcceptor.h"
#include "Framework/Paxos/PaxosLearner.h"
#include "Framework/PaxosLease/PaxosLease.h"
#include "Framework/ReplicatedDB/ReplicatedDB.h"
#include "ReplicatedLogMsg.h"
#include "LogCache.h"
#include "LogQueue.h"

#define CATCHUP_TIMEOUT	5000

class ReplicatedLog : private PaxosProposer, private PaxosAcceptor, private PaxosLearner
{
public:
	ReplicatedLog();

	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_, char* filename);
	
	bool				Append(ByteString value);
	
	void				SetReplicatedDB(ReplicatedDB* replicatedDB_);
	
	LogItem*			LastLogItem();
	
	bool				IsMaster();
	
	int					NodeID();
	
	bool				Stop();
	bool				Continue();

private:
	virtual void		OnPrepareRequest();
	virtual void		OnPrepareResponse();
	virtual void		OnProposeRequest();
	virtual void		OnProposeResponse();
	virtual void		OnLearnChosen();
	virtual void		OnRequestChosen();

	void				OnRequest();
	
	void				OnCatchupTimeout();
	
	void				OnLearnLease();
	void				OnLeaseTimeout();
	
	void				NewPaxosRound();

	IOProcessor*		ioproc;
	Scheduler*			scheduler;
	
	PaxosLease			masterLease;

	bool				appending;
	ReplicatedLogMsg	msg;
	ByteArray<65*KB>	value;
	
	ulong64				highestPaxosID;
	
	LogCache			logCache;
	LogQueue			logQueue;
	
	MFunc<ReplicatedLog>onCatchupTimeout;
	CdownTimer			catchupTimeout;
	
	MFunc<ReplicatedLog>onLearnLease;
	MFunc<ReplicatedLog>onLeaseTimeout;

	ReplicatedDB*		replicatedDB;
	
	PaxosConfig			config;
};
#endif
