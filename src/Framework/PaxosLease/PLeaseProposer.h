#ifndef PLEASEPROPOSER_H
#define PLEASEPROPOSER_H

#include "System/Common.h"
#include "System/Events/Scheduler.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class ReplicatedLog;

class PLeaseProposer
{
public:
	PLeaseProposer();

	void					Init(ReplicatedLog* replicatedLog_, TransportWriter** writers_,
								 Scheduler* scheduler_);
	
	void					ProcessMsg(PLeaseMsg &msg_);
	void					OnAcquireLeaseTimeout();
	void					OnExtendLeaseTimeout();
	void					AcquireLease();

private:
	void					BroadcastMessage();
	void					OnPrepareResponse();
	void					OnProposeResponse();
	void					StartPreparing();
	void					StartProposing();

	ReplicatedLog*			replicatedLog;
	TransportWriter**		writers;
	Scheduler*				scheduler;
	ByteArray<PLEASE_BUFSIZE>wdata;
	PaxosConfig*			config;
	PLeaseProposerState		state;
	PLeaseMsg				msg;
	MFunc<PLeaseProposer>	onAcquireLeaseTimeout;
	CdownTimer				acquireLeaseTimeout;
	MFunc<PLeaseProposer>	onExtendLeaseTimeout;
	Timer					extendLeaseTimeout;
// keeping track of messages during prepare and propose phases
	int						numReceived;
	int						numAccepted;
	int						numRejected;
};

#endif
