#ifndef PLEASEPROPOSER_H
#define PLEASEPROPOSER_H

#include "System/Common.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class PLeaseProposer
{
public:
							PLeaseProposer();

	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
		
	void					OnRead();
	void					OnWrite();
	
	void					OnAcquireLeaseTimeout();
	void					OnExtendLeaseTimeout();
	
	void					AcquireLease();

private:
	void					BroadcastMessage();
	void					SendMessage();

	void					ProcessMsg();
	virtual void			OnPrepareResponse();
	virtual void			OnProposeResponse();

	void					StartPreparing();
	void					StartProposing();

	bool					TryFinishPreparing();
	bool					TryFinishProposing();

	IOProcessor*			ioproc;
	Scheduler*				scheduler;
	Socket					socket;

	UDPRead					udpread;
	UDPWrite				udpwrite;
	
	ByteArray<64*KB>		rdata;
	ByteArray<64*KB>		wdata;
	
	PaxosConfig*			config;

	PLeaseProposerState		state;
	
	Endpoint*				sending_to;
	
	PLeaseMsg				msg;
	
	MFunc<PLeaseProposer>	onRead;
	MFunc<PLeaseProposer>	onWrite;
	
	MFunc<PLeaseProposer>	onAcquireLeaseTimeout;
	CdownTimer				acquireLeaseTimeout;
	
	MFunc<PLeaseProposer>	onExtendLeaseTimeout;
	Timer					extendLeaseTimeout;
	
// keeping track of messages during prepare and propose phases
	int						numSent;
	int						numReceived;
	int						numAccepted;
	int						numRejected;
};

#endif
