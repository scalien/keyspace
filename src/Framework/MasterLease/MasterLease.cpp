#include "MasterLease.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include <assert.h>
#include <stdlib.h>

MasterLease::MasterLease() :
//onAppend(this, &MasterLease::OnAppend),
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

bool MasterLease::Init(IOProcessor* ioproc_, Scheduler* scheduler_,
		ReplicatedLog* replicatedLog_)
{
	replicatedLog = replicatedLog_;

	scheduler = scheduler_;
	
	masterID = MASTER_UNKNOWN;
	srand(Now());
	epochID = rand(); // TODO: persist

	designated = (replicatedLog->NodeID() == 0);
	if (designated)
		Log_Message("designated!");
	
	// start timeouts
	if (designated)
	{
		msg.YieldLease(replicatedLog->NodeID(), epochID);
		MasterLeaseMsg::Write(&msg, data);
		replicatedLog->Append(data);
	}
	
	scheduler->Add(&recvLeaseExtendTimeout);

	Log_Trace();
	scheduler->Add(&report);
	
	master = false;
	replicatedLog->SetMaster(master);	
	return true;
}

void MasterLease::OnAppend(Transaction*, Entry* entry)
{
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
	
	replicatedLog->Cancel();
	
	if (replicatedLog->NodeID() == masterID && msg.epochID == epochID)
	{
		master = true;
		replicatedLog->SetMaster(master);
		
		Log_Message(
		"+++ node %d thinks it is the master +++", replicatedLog->NodeID(), masterID
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
			msg.YieldLease(replicatedLog->NodeID(), epochID);
			MasterLeaseMsg::Write(&msg, data);
			replicatedLog->Append(data);
		}
	}
	
	recvLeaseExtendTimeout.delay = RECV_LEASE_EXTEND_TIMEOUT;
	scheduler->Reset(&recvLeaseExtendTimeout);
}

void MasterLease::YieldLease()
{
	Log_Trace();
	
	// received a 'yield lease' msg
	
	if (msg.nodeID == replicatedLog->NodeID() && msg.epochID == epochID)
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
	replicatedLog->SetMaster(master);
	
	masterID = MASTER_UNKNOWN;
}

void MasterLease::OnSendLeaseExtendTimeout()
{
	Log_Trace();
	
	// node is the master and needs to send the 'lease extend' message
	
	assert(replicatedLog->NodeID() == masterID);
	
	msg.ExtendLease(replicatedLog->NodeID(), epochID);
	MasterLeaseMsg::Write(&msg, data);
	replicatedLog->Append(data);
}

void MasterLease::OnRecvLeaseExtendTimeout()
{
	Log_Trace();
	
	// node has not received a 'lease extend' message for a while
	// old master is probably down
	// volunteer to become the new master
	
	masterID = MASTER_UNKNOWN;
	
	msg.ExtendLease(replicatedLog->NodeID(), epochID);
	MasterLeaseMsg::Write(&msg, data);
	replicatedLog->Append(data);
	
	recvLeaseExtendTimeout.delay = RECV_LEASE_EXTEND_TIMEOUT;
	scheduler->Reset(&recvLeaseExtendTimeout);
}

void MasterLease::OnReport()
{
	if (master)
		Log_Message("+++ node %d thinks it is the master +++", replicatedLog->NodeID());
	else
		Log_Message("+++ node %d thinks the master may be %d +++",
			replicatedLog->NodeID(), masterID);

	scheduler->Reset(&report);
}
