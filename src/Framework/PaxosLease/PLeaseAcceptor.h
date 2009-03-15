#ifndef PLEASEACCEPTOR_H
#define PLEASEACCEPTOR_H

#include "System/Common.h"
#include "System/Events/Scheduler.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class PLeaseAcceptor
{
public:
							PLeaseAcceptor();
	
	bool					Init(TransportWriter** writers_, Scheduler* scheduler_, PaxosConfig* config_);

	void					ProcessMsg(PLeaseMsg &msg_);
	
	void					OnLeaseTimeout();

private:
	void					SendReply(unsigned nodeID);

	virtual void			OnPrepareRequest();
	virtual void			OnProposeRequest();

	TransportWriter**		writers;
	Scheduler*				scheduler;

	ByteArray<64*KB>		wdata;
	
	MFunc<PLeaseAcceptor>	onLeaseTimeout;
	Timer					leaseTimeout;
	
	PLeaseMsg				msg;
	
	PaxosConfig*			config;
	PLeaseAcceptorState		state;
};

#endif
