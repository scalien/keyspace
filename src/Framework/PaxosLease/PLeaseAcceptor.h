#ifndef PLEASEACCEPTOR_H
#define PLEASEACCEPTOR_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportUDPWriter.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class ReplicatedLog;

class PLeaseAcceptor
{
public:
	PLeaseAcceptor();
	
	void					Init(TransportUDPWriter** writers_);
	void					ProcessMsg(PLeaseMsg &msg_);
	void					OnLeaseTimeout();

private:
	void					SendReply(unsigned nodeID);

	void					OnPrepareRequest();
	void					OnProposeRequest();

	TransportUDPWriter**	writers;
	ByteBuffer				wdata;
	MFunc<PLeaseAcceptor>	onLeaseTimeout;
	Timer					leaseTimeout;
	PLeaseMsg				msg;
	PLeaseAcceptorState		state;
};

#endif
