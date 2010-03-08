#ifndef PAXOSACCEPTOR_H
#define PAXOSACCEPTOR_H

#include "System/Common.h"
#include "Framework/Transport/TransportTCPWriter.h"
#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "Framework/Database/Transaction.h"
#include "PaxosMsg.h"
#include "PaxosState.h"

class PaxosAcceptor
{
	friend class ReplicatedLog;
	typedef TransportTCPWriter**	Writers;
	typedef PaxosAcceptorState		State;
public:

	void			Init(Writers writer_);
	void			Shutdown();
	void			WriteState(Transaction* transaction);
	
protected:
	bool			ReadState();
	void			SendReply(unsigned nodeID);
	void			OnPrepareRequest(PaxosMsg& msg_);
	void			OnProposeRequest(PaxosMsg& msg_);

	Writers			writers;
	ByteBuffer		msgbuf;
	uint64_t		paxosID;
	PaxosMsg		msg;
	State			state;
	Table*			table;
	Transaction		transaction;
	ByteArray<128>	buffers[4];
};

#endif
