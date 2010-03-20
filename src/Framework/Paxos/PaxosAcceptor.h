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
	typedef	MFunc<PaxosAcceptor>	Func;
public:
	PaxosAcceptor();
	
	void			Init(Writers writer_);
	void			Shutdown();
	bool			Persist(Transaction* transaction);
	bool			IsWriting() { return mdbop.IsActive(); }
	
protected:
	bool			WriteState();
	bool			ReadState();
	void			SendReply(unsigned nodeID);
	void			OnPrepareRequest(PaxosMsg& msg_);
	void			OnProposeRequest(PaxosMsg& msg_);
	void			OnDBComplete();

	Writers			writers;
	ByteBuffer		msgbuf;
	uint64_t		paxosID;
	PaxosMsg		msg;
	unsigned		senderID;
	State			state;
	Table*			table;
	Transaction		transaction;
	MultiDatabaseOp	mdbop;
	ByteArray<128>	buffers[4];
	uint64_t		writtenPaxosID;
	Func			onDBComplete;
};

#endif
