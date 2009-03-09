#include "TestDB.h"

#define PROTOCOL_TESTDB		'T'

void TestDB::Init(IOProcessor*, Scheduler*, ReplicatedLog* replicatedLog_)
{
	replicatedLog = replicatedLog_;
	seq = 0;
	master = false;
	
	replicatedLog->SetReplicatedDB(this);
}

void TestDB::OnAppend(Transaction*, LogItem*)
{
	Log_Trace();
	
	if (replicatedLog->IsMaster())
	{
		ba.length = snprintf(ba.data, ba.size,
			"%c:%d:%d", PROTOCOL_TESTDB, replicatedLog->NodeID(), seq++);
		
		Log_Message("Proposing value %.*s", ba.length, ba.buffer);
		
		replicatedLog->Append(ba);
	}
}

void TestDB::OnMasterLease(int nodeID)
{
	Log_Trace();
	
	if (!master && nodeID == replicatedLog->NodeID())
	{
		master = true;
		OnAppend(NULL, NULL);
	}
}
