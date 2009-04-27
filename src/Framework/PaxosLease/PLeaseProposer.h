#ifndef PLEASEPROPOSER_H
#define PLEASEPROPOSER_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportWriter.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class ReplicatedLog;

class PLeaseProposer
{
public:
	PLeaseProposer();

	void					Init(TransportWriter** writers_);
	
	void					ProcessMsg(PLeaseMsg &msg_);
	void					OnAcquireLeaseTimeout();
	void					OnExtendLeaseTimeout();
	void					AcquireLease();

	uint64_t				highestProposalID;

private:
	void					BroadcastMessage();
	void					OnPrepareResponse();
	void					OnProposeResponse();
	void					StartPreparing();
	void					StartProposing();

	TransportWriter**		writers;
	ByteArray<PLEASE_BUFSIZE>wdata;
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
