#ifndef REPLICATEDLOG_H
#define REPLICATEDLOG_H

#include "System/Containers/List.h"
#include "System/Events/Callable.h"
#include "Framework/Transport/TransportTCPReader.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "Framework/Paxos/PaxosProposer.h"
#include "Framework/Paxos/PaxosAcceptor.h"
#include "Framework/Paxos/PaxosLearner.h"
#include "Framework/PaxosLease/PaxosLease.h"
#include "Framework/ReplicatedLog/ReplicatedDB.h"
#include "ReplicatedLogMsg.h"
#include "LogCache.h"
#include "LogQueue.h"

#define CATCHUP_TIMEOUT	5000

#define	RLOG (ReplicatedLog::Get())

class ReplicatedLog
{
	typedef TransportTCPReader*		Reader;
	typedef TransportTCPWriter**	Writers;
	typedef MFunc<ReplicatedLog>	Func;
public:
	ReplicatedLog();

	static ReplicatedLog* Get();
	
	bool				Init(bool useSoftClock);
	void				Shutdown();
	
	void				OnRead();
	bool				Append(ByteString &value);
	void				SetReplicatedDB(ReplicatedDB* replicatedDB_);
	Transaction*		GetTransaction();
	bool				GetLogItem(uint64_t paxosID, ByteString& value);
	uint64_t			GetPaxosID();
	void				SetPaxosID(Transaction* transaction, uint64_t paxosID);
	bool				IsMaster();
	int					GetMaster();
	unsigned 			GetNodeID();
	void				StopPaxos();
	void				StopMasterLease();
	void				StopReplicatedDB();
	void				ContinuePaxos();
	void				ContinueMasterLease();
	void				ContinueReplicatedDB();
	bool				IsPaxosActive();
	bool				IsMasterLeaseActive();
	bool				IsAppending();
	bool				IsSafeDB();
	void				OnPaxosLeaseMsg(uint64_t paxosID, unsigned nodeID);

private:
	void				InitTransport();
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

	Reader				reader;
	Writers				writers;
	Func				onRead;
	PaxosProposer		proposer;
	PaxosAcceptor		acceptor;
	PaxosLearner		learner;
	PaxosLease			masterLease;
	PaxosMsg			pmsg;
	ReplicatedLogMsg	rmsg;
	ByteBuffer			value;
	uint64_t			highestPaxosID;
	LogCache			logCache;
	LogQueue			logQueue;
	Func				onCatchupTimeout;
	CdownTimer			catchupTimeout;
	Func				onLearnLease;
	Func				onLeaseTimeout;
	ReplicatedDB*		replicatedDB;
	bool				safeDB;
};
#endif
