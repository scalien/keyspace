#ifndef PAXOSPROPOSER_H
#define PAXOSPROPOSER_H

#include "System/Common.h"
#include "System/Events/Scheduler.h"
#include "Framework/Transport/TransportWriter.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

class PaxosProposer
{
friend class ReplicatedLog;

public:
	PaxosProposer();

	void					Init(TransportWriter** writers_, Scheduler* scheduler_);
		
	void					OnPrepareTimeout();
	void					OnProposeTimeout();
	
	void					SetPaxosID(ulong64 paxosID_);

	bool					IsActive();	
	bool					Propose(ByteString& value);

protected:
	void					BroadcastMessage();

	void					OnPrepareResponse(PaxosMsg& msg_);
	void					OnProposeResponse(PaxosMsg& msg_);

	void					StopPreparing();
	void					StopProposing();

	void					StartPreparing();
	void					StartProposing();

	TransportWriter**		writers;
	Scheduler*				scheduler;
	
	ByteArray<PAXOS_BUFSIZE>wdata;
	
	PaxosConfig*			config;
	PaxosProposerState		state;
	
	PaxosMsg				msg;

	ulong64					paxosID;
	
	MFunc<PaxosProposer>	onPrepareTimeout;
	MFunc<PaxosProposer>	onProposeTimeout;
	CdownTimer				prepareTimeout;
	CdownTimer				proposeTimeout;
	
// keeping track of messages during prepare and propose phases
	int						numReceived;
	int						numAccepted;
	int						numRejected;
};

#endif
