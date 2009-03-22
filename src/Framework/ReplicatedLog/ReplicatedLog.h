#ifndef REPLICATEDLOG_H
#define REPLICATEDLOG_H

#include "System/Containers/List.h"
#include "System/Events/Callable.h"
#include "Framework/Transport/TransportReader.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/Paxos/PaxosProposer.h"
#include "Framework/Paxos/PaxosAcceptor.h"
#include "Framework/Paxos/PaxosLearner.h"
#include "Framework/PaxosLease/PaxosLease.h"
#include "Framework/ReplicatedDB/ReplicatedDB.h"
#include "ReplicatedLogMsg.h"
#include "LogCache.h"
#include "LogQueue.h"

#define CATCHUP_TIMEOUT	5000

class ReplicatedLog
{
public:
	ReplicatedLog();

	bool				Init(IOProcessor* ioproc_, Scheduler* scheduler_, char* filename);

	void				OnRead();
	
	bool				Append(ByteString value);
	
	void				SetReplicatedDB(ReplicatedDB* replicatedDB_);
	
	LogItem*			LastLogItem();
	LogItem*			GetLogItem(ulong64 paxosID);

	ulong64				GetPaxosID();
	bool				IsMaster();	
	int					NodeID();
	
	bool				Stop();
	bool				Continue();

private:
	void				InitTransport(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void				ProcessMsg();
	void				OnPrepareRequest();
	void				OnPrepareResponse();
	void				OnProposeRequest();
	void				OnProposeResponse();
	void				OnLearnChosen();
	void				OnRequestChosen();

	void				OnRequest();
	
	void				OnCatchupTimeout();
	
	void				OnLearnLease();
	void				OnLeaseTimeout();
	
	void				NewPaxosRound();

	Scheduler*			scheduler;
	TransportReader*	reader;
	TransportWriter**	writers;

	MFunc<ReplicatedLog>onRead;
	
	PaxosProposer		proposer;
	PaxosAcceptor		acceptor;
	PaxosLearner		learner;
	
	PaxosLease			masterLease;

	bool				appending;
	PaxosMsg			pmsg;
	ReplicatedLogMsg	rmsg;
	ByteArray<PAXOS_BUFSIZE>value;
	
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
