#ifndef PAXOSPROPOSER_H
#define PAXOSPROPOSER_H

#include "System/Common.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "PaxosMsg.h"
#include "PaxosState.h"
#include "PaxosConfig.h"

class PaxosProposer
{
public:
							PaxosProposer();

	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_, PaxosConfig* config_);
		
	void					OnRead();
	void					OnWrite();
	
	void					OnPrepareTimeout();
	void					OnProposeTimeout();
	
	void					SetPaxosID(ulong64 paxosID_);

	bool					IsActive();	
	bool					Propose(ByteString& value);

protected:
	void					BroadcastMessage();
	void					SendMessage();

	void					ProcessMsg();
	virtual void			OnPrepareResponse();
	virtual void			OnProposeResponse();

	void					StopPreparing();
	void					StopProposing();

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
	PaxosProposerState		state;
	
	Endpoint*				sending_to;
	
	PaxosMsg				msg;

	ulong64					paxosID;
	
	MFunc<PaxosProposer>	onRead;
	MFunc<PaxosProposer>	onWrite;

	MFunc<PaxosProposer>	onPrepareTimeout;
	MFunc<PaxosProposer>	onProposeTimeout;
	CdownTimer				prepareTimeout;
	CdownTimer				proposeTimeout;
	
// keeping track of messages during prepare and propose phases
	int						numSent;
	int						numReceived;
	int						numAccepted;
	int						numRejected;
};

#endif
