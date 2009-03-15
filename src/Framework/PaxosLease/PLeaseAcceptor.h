#ifndef PLEASEACCEPTOR_H
#define PLEASEACCEPTOR_H

#include "System/Common.h"
#include "System/Events/Scheduler.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseConsts.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class PLeaseAcceptor
{
public:
	PLeaseAcceptor();
	
	void					Init(TransportWriter** writers_, Scheduler* scheduler_, PaxosConfig* config_);

	void					ProcessMsg(PLeaseMsg &msg_);
	
	void					OnLeaseTimeout();

private:
	void					SendReply(unsigned nodeID);

	void					OnPrepareRequest();
	void					OnProposeRequest();

	TransportWriter**		writers;
	Scheduler*				scheduler;

	ByteArray<PLEASE_BUFSIZE>wdata;
	
	MFunc<PLeaseAcceptor>	onLeaseTimeout;
	Timer					leaseTimeout;
	
	PLeaseMsg				msg;
	
	PaxosConfig*			config;
	PLeaseAcceptorState		state;
};

#endif
