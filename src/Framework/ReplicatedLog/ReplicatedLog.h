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
#define USE_TCP			0


class ReplicatedLog
{
public:
	ReplicatedLog();

	static ReplicatedLog*		Get();
	bool						Init();
	void						OnRead();
	bool						Append(ByteString value);
	void						SetReplicatedDB(ReplicatedDB* replicatedDB_);
	Transaction*				GetTransaction();
	bool						GetLogItem(ulong64 paxosID, ByteString& value);
	ulong64						GetPaxosID();
	void						SetPaxosID(Transaction* transaction, ulong64 paxosID);
	bool						IsMaster();	
	int							GetNodeID();
	void						Stop();
	void						Continue();
	bool						IsAppending();
	bool						IsSafeDB();

private:
	void						InitTransport();
	void						ProcessMsg();
	void						OnPrepareRequest();
	void						OnPrepareResponse();
	void						OnProposeRequest();
	void						OnProposeResponse();
	void						OnLearnChosen();
	void						OnRequestChosen();
	void						OnRequest();
	void						OnCatchupTimeout();
	void						OnLearnLease();
	void						OnLeaseTimeout();
	void						NewPaxosRound();

	TransportReader*			reader;
	TransportWriter**			writers;
	MFunc<ReplicatedLog>		onRead;
	PaxosProposer				proposer;
	PaxosAcceptor				acceptor;
	PaxosLearner				learner;
	PaxosLease					masterLease;
	bool						appending;
	PaxosMsg					pmsg;
	ReplicatedLogMsg			rmsg;
	ByteArray<PAXOS_BUFSIZE>	value;
	ulong64						highestPaxosID;
	LogCache					logCache;
	LogQueue					logQueue;
	MFunc<ReplicatedLog>		onCatchupTimeout;
	CdownTimer					catchupTimeout;
	MFunc<ReplicatedLog>		onLearnLease;
	MFunc<ReplicatedLog>		onLeaseTimeout;
	ReplicatedDB*				replicatedDB;
	bool						safeDB;
};
#endif
