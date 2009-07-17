#ifndef PAXOSPROPOSER_H
#define PAXOSPROPOSER_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "PaxosMsg.h"
#include "PaxosState.h"

#define PAXOS_TIMEOUT	3000 // TODO: I increased this for testing

class PaxosProposer
{
	friend class ReplicatedLog;
	typedef TransportTCPWriter**	Writers;
	typedef MFunc<PaxosProposer>	Func;
	typedef PaxosProposerState		State;
public:
	PaxosProposer();

	void		Init(TransportTCPWriter** writers_);
		
	void		OnPrepareTimeout();
	void		OnProposeTimeout();
	bool		IsActive();	
	bool		Propose(ByteString& value);
	void		Stop();

protected:
	void		BroadcastMessage();
	void		OnPrepareResponse(PaxosMsg& msg_);
	void		OnProposeResponse(PaxosMsg& msg_);
	void		StopPreparing();
	void		StopProposing();
	void		StartPreparing();
	void		StartProposing();

	Writers		writers;
	ByteBuffer	wdata;
	State		state;
	PaxosMsg	msg;
	uint64_t	paxosID;
	Func		onPrepareTimeout;
	Func		onProposeTimeout;
	CdownTimer	prepareTimeout;
	CdownTimer	proposeTimeout;
// keeping track of messages during prepare and propose phases
	unsigned	numReceived;
	unsigned	numAccepted;
	unsigned	numRejected;
};

#endif
