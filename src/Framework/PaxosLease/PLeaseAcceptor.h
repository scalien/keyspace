#ifndef PLEASEACCEPTOR_H
#define PLEASEACCEPTOR_H

#include "System/Common.h"
#include "System/Events/Timer.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class ReplicatedLog;

class PLeaseAcceptor
{
	typedef TransportTCPWriter**	Writers;
	typedef MFunc<PLeaseAcceptor>	Func;
	typedef PLeaseAcceptorState		State;
	
public:
	PLeaseAcceptor();
	
	void		Init(Writers writers_);
	void		ProcessMsg(PLeaseMsg &msg_);
	void		OnLeaseTimeout();

private:
	void		SendReply(unsigned nodeID);

	void		OnPrepareRequest();
	void		OnProposeRequest();

	Writers		writers;
	ByteBuffer	wdata;
	Func		onLeaseTimeout;
	Timer		leaseTimeout;
	PLeaseMsg	msg;
	State		state;
};

#endif
