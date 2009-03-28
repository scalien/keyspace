#ifndef PLEASEACCEPTOR_H
#define PLEASEACCEPTOR_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportWriter.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class ReplicatedLog;

class PLeaseAcceptor
{
public:
	PLeaseAcceptor();
	
	void					Init(TransportWriter** writers_);
	void					ProcessMsg(PLeaseMsg &msg_);
	void					OnLeaseTimeout();

private:
	void					SendReply(unsigned nodeID);

	void					OnPrepareRequest();
	void					OnProposeRequest();

	TransportWriter**		writers;
	ByteArray<PLEASE_BUFSIZE>wdata;
	MFunc<PLeaseAcceptor>	onLeaseTimeout;
	Timer					leaseTimeout;
	PLeaseMsg				msg;
	PLeaseAcceptorState		state;
};

#endif
