#ifndef PLEASEPROPOSER_H
#define PLEASEPROPOSER_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class ReplicatedLog;

class PLeaseProposer
{
public:
	PLeaseProposer();

	void					Init(TransportUDPWriter** writers_);
	
	void					ProcessMsg(PLeaseMsg &msg_);
	void					OnNewPaxosRound();
	void					OnAcquireLeaseTimeout();
	void					OnExtendLeaseTimeout();
	void					StartAcquireLease();
	void					StopAcquireLease();

	uint64_t				highestProposalID;

private:
	void					BroadcastMessage();
	void					OnPrepareResponse();
	void					OnProposeResponse();
	void					StartPreparing();
	void					StartProposing();

	TransportUDPWriter**	writers;
	ByteBuffer				wdata;
	PLeaseProposerState		state;
	PLeaseMsg				msg;
	MFunc<PLeaseProposer>	onAcquireLeaseTimeout;
	CdownTimer				acquireLeaseTimeout;
	MFunc<PLeaseProposer>	onExtendLeaseTimeout;
	Timer					extendLeaseTimeout;
// keeping track of messages during prepare and propose phases
	unsigned				numReceived;
	unsigned				numAccepted;
	unsigned				numRejected;
};

#endif
