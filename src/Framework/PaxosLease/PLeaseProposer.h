#ifndef PLEASEPROPOSER_H
#define PLEASEPROPOSER_H

#include "System/Common.h"
#include "System/Events/Scheduler.h"
#include "Framework/Transport/TransportWriter.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "PLeaseMsg.h"
#include "PLeaseState.h"

class PLeaseProposer
{
public:
							PLeaseProposer();

	bool					Init(TransportWriter** writers_, Scheduler* scheduler_, PaxosConfig* config_);

	void					ProcessMsg(PLeaseMsg &msg_);
			
	void					OnAcquireLeaseTimeout();
	void					OnExtendLeaseTimeout();
	
	void					AcquireLease();

private:
	void					BroadcastMessage();

	virtual void			OnPrepareResponse();
	virtual void			OnProposeResponse();

	void					StartPreparing();
	void					StartProposing();

	TransportWriter**		writers;
	Scheduler*				scheduler;
	
	ByteArray<64*KB>		wdata;
	
	PaxosConfig*			config;

	PLeaseProposerState		state;
	
	PLeaseMsg				msg;
		
	MFunc<PLeaseProposer>	onAcquireLeaseTimeout;
	CdownTimer				acquireLeaseTimeout;
	
	MFunc<PLeaseProposer>	onExtendLeaseTimeout;
	Timer					extendLeaseTimeout;
	
// keeping track of messages during prepare and propose phases
	int						numReceived;
	int						numAccepted;
	int						numRejected;
};

#endif
