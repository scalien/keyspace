#ifndef PLEASEACCEPTOR_H
#define PLEASEACCEPTOR_H

#include "System/Common.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"
#include "PaxosConfig.h"

class PLeaseAcceptor
{
public:
							PLeaseAcceptor();
	
	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
	
	void					OnRead();
	void					OnWrite();
	
	void					OnLeaseTimeout();

private:
	void					SendReply();

	void					ProcessMsg();
	virtual void			OnPrepareRequest();
	virtual void			OnProposeRequest();

	IOProcessor*			ioproc;
	Scheduler*				scheduler;
	Socket					socket;

	UDPRead					udpread;
	UDPWrite				udpwrite;
	
	ByteArray<64*KB>		rdata;
	ByteArray<64*KB>		wdata;
	
	MFunc<PLeaseAcceptor>	onRead;
	MFunc<PLeaseAcceptor>	onWrite;
	
	MFunc<PLeaseAcceptor>	onLeaseTimeout;
	Timer					leaseTimeout;
	
	PLeaseMsg				msg;
	
	PaxosConfig*			config;
	PLeaseAcceptorState		state;
};

#endif