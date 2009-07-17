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
	typedef TransportUDPWriter**	Writers;
	typedef MFunc<PLeaseProposer>	Func;
	typedef PLeaseProposerState		State;
	
public:
	PLeaseProposer();

	void		Init(Writers writers_);
	
	void		ProcessMsg(PLeaseMsg &msg_);
	void		OnNewPaxosRound();
	void		OnAcquireLeaseTimeout();
	void		OnExtendLeaseTimeout();
	void		StartAcquireLease();
	void		StopAcquireLease();

	uint64_t	highestProposalID;

private:
	void		BroadcastMessage();
	void		OnPrepareResponse();
	void		OnProposeResponse();
	void		StartPreparing();
	void		StartProposing();

	Writers		writers;
	ByteBuffer	wdata;
	State		state;
	PLeaseMsg	msg;
	Func		onAcquireLeaseTimeout;
	CdownTimer	acquireLeaseTimeout;
	Func		onExtendLeaseTimeout;
	Timer		extendLeaseTimeout;
// keeping track of messages during prepare and propose phases
	unsigned	numReceived;
	unsigned	numAccepted;
	unsigned	numRejected;
};

#endif
