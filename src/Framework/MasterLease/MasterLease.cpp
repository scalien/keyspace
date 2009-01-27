#include "MasterLease.h"
#include <assert.h>
#include <stdlib.h>

MasterLease::MasterLease() :
onAppend(this, &MasterLease::OnAppend),
onMasterTimeout(this, &MasterLease::OnMasterTimeout),
onSendLeaseExtendTimeout(this, &MasterLease::OnSendLeaseExtendTimeout),
onRecvLeaseExtendTimeout(this, &MasterLease::OnRecvLeaseExtendTimeout),
onReport(this, &MasterLease::OnReport),
masterTimeout(MASTER_TIMEOUT, &onMasterTimeout),
sendLeaseExtendTimeout(SEND_LEASE_EXTEND_TIMEOUT, &onSendLeaseExtendTimeout),
recvLeaseExtendTimeout(RECV_LEASE_EXTEND_TIMEOUT, &onRecvLeaseExtendTimeout),
report(500, &onReport)
{
}

bool MasterLease::Init(IOProcessor* ioproc_, Scheduler* scheduler_)
{
	ReplicatedLog::Init(ioproc_, scheduler_);

	ReplicatedLog::appendCallback = &onAppend;

	scheduler = scheduler_;
	
	masterID = MASTER_UNKNOWN;
	srand(Now());
	epochID = rand(); // todo: just a hack

	designated = (config.nodeID == 0);
	if (designated)
		Log_Message("designated!");
	
	// start timeouts
	if (designated)
	{
		msg.YieldLease(config.nodeID, epochID);
		MasterLeaseMsg::Write(&msg, data);
		ReplicatedLog::Append(data);
	}
	
	scheduler->Add(&recvLeaseExtendTimeout);

	Log_Trace();
	scheduler->Add(&report);
	
	master = false;
	
	return true;
}

void MasterLease::OnAppend()
{
	// called back by the replicated log when a new value is appended 
	// this may not be ours

	Entry* entry;
	
	entry = memlog.Head();
	
	Execute(entry->value);
}

void MasterLease::Execute(ByteString command)
{
	MasterLeaseMsg::Read(command, &msg);
	
	if (msg.type == EXTEND_LEASE)
		ExtendLease();
	else if (msg.type == YIELD_LEASE)
		YieldLease();
}

void MasterLease::ExtendLease()
{
	Log_Trace();
	
	masterID = msg.nodeID;
	
	if (ReplicatedLog::appending)
		ReplicatedLog::Remove();
	
	if (config.nodeID == masterID && msg.epochID == epochID)
	{
		master = true;
		
		Log_Message(
		"+++ node %d thinks it is the master +++", config.nodeID, masterID
		);
	
		// node is the master
		scheduler->Reset(&sendLeaseExtendTimeout);
		scheduler->Reset(&masterTimeout);
	}
	else
	{
		// node is *not* the master
		
		if (designated)
		{
			msg.YieldLease(config.nodeID, epochID);
			MasterLeaseMsg::Write(&msg, data);
			ReplicatedLog::Append(data);
		}
	}
	
	recvLeaseExtendTimeout.delay = RECV_LEASE_EXTEND_TIMEOUT;
	scheduler->Reset(&recvLeaseExtendTimeout);
}

void MasterLease::YieldLease()
{
	Log_Trace();
	
	// received a 'yield lease' msg
	
	if (msg.nodeID == config.nodeID && msg.epochID == epochID)
	{
		// this node's yield msg
		
		// just wait for my RecvLeaseExtendTimeout to happen
		// at that point I will become the master
	}
	else
	{
		// other node's yield msg
		
		// remove my scheduled ExtendTimeout msg, if any
		scheduler->Remove(&sendLeaseExtendTimeout);
		
		// wait for YIELD_TIMEOUT msecs for someone to claim mastership
		recvLeaseExtendTimeout.delay = YIELD_TIMEOUT;
		scheduler->Reset(&recvLeaseExtendTimeout);
	}
}

void MasterLease::OnMasterTimeout()
{
	Log_Trace();
	
	// master was unable to extend its lease
	master = false;
	
	masterID = MASTER_UNKNOWN;
}

void MasterLease::OnSendLeaseExtendTimeout()
{
	Log_Trace();
	
	// node is the master and needs to send the 'lease extend' message
	
	assert(config.nodeID == masterID);
	
	msg.ExtendLease(config.nodeID, epochID);
	MasterLeaseMsg::Write(&msg, data);
	ReplicatedLog::Append(data);
}

void MasterLease::OnRecvLeaseExtendTimeout()
{
	Log_Trace();
	
	// node has not received a 'lease extend' message for a while
	// old master is probably down
	// volunteer to become the new master
	
	masterID = MASTER_UNKNOWN;
	
	msg.ExtendLease(config.nodeID, epochID);
	MasterLeaseMsg::Write(&msg, data);
	ReplicatedLog::Append(data);
	
	recvLeaseExtendTimeout.delay = RECV_LEASE_EXTEND_TIMEOUT;
	scheduler->Reset(&recvLeaseExtendTimeout);
}

void MasterLease::OnReport()
{
	if (master)
		Log_Message("+++ node %d thinks it is the master +++", config.nodeID);
	else
		Log_Message("+++ node %d thinks the master may be %d +++",
			config.nodeID, masterID);

	scheduler->Reset(&report);
}
