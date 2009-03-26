#include "TestDB.h"

#define PROTOCOL_TESTDB		'T'

void TestDB::Init(IOProcessor*, Scheduler*, ReplicatedLog* replicatedLog_)
{
	replicatedLog = replicatedLog_;
	seq = 0;
	master = false;
	
	replicatedLog->SetReplicatedDB(this);
}

void TestDB::OnAppend(Transaction*, ulong64, ByteString, bool)
{
	Log_Trace();
	
	if (replicatedLog->IsMaster())
	{
		ba.length = snprintf(ba.data, ba.size,
			"%c:%d:%d", PROTOCOL_TESTDB, replicatedLog->GetNodeID(), seq++);
		
		Log_Message("Proposing value %.*s", ba.length, ba.buffer);
		
		replicatedLog->Append(ba);
	}
}

void TestDB::OnMasterLease(unsigned nodeID)
{
	Log_Trace();
	
	if (!master && nodeID == replicatedLog->GetNodeID())
	{
		master = true;
		OnAppend(NULL, 0, ByteString(), false);
	}
}
