#include "CatchupReader.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "Application/Keyspace/Database/ReplicatedKeyspaceDB.h"

void CatchupReader::Init(ReplicatedKeyspaceDB* keyspaceDB_, Table* table_)
{
	keyspaceDB = keyspaceDB_;
	table = table_;
	
	transaction.Set(table);
}

void CatchupReader::Start(unsigned nodeID, uint64_t paxosID)
{
	Log_Trace();

	ReplicatedLog::Get()->StopPaxos();

	bool ret;
	Endpoint endpoint;
/*	
	ret = true;
	ret &= transaction.Begin();
	ret &= table->Truncate(&transaction);
	if (!ret)
		ASSERT_FAIL();
*/
	// this is a workaround because BDB truncate is way too slow for any
	// database bigger than 1Gb
	if (paxosID != 0)
		RESTART("exiting to truncate database");

	endpoint = ReplicatedConfig::Get()->endpoints[nodeID];
	endpoint.SetPort(endpoint.GetPort() + CATCHUP_PORT_OFFSET);
	Connect(endpoint, CATCHUP_CONNECT_TIMEOUT);
}

void CatchupReader::OnMessageRead(const ByteString& message)
{
	if (msg.Read(message))
		ProcessMsg();
}

void CatchupReader::OnClose()
{
	Log_Trace();

	if (state == DISCONNECTED)
		return;

	if (transaction.IsActive())
		transaction.Rollback();
	
	keyspaceDB->OnCatchupFailed();
	
	Close();
	
	ReplicatedLog::Get()->ContinuePaxos();
}

void CatchupReader::OnConnect()
{
	Log_Trace();

	TCPConn<>::OnConnect();
	
	AsyncRead();
}

void CatchupReader::OnConnectTimeout()
{
	Log_Trace();

	OnClose();
}

void CatchupReader::ProcessMsg()
{
//	Log_Trace();

	if (msg.type == CATCHUP_KEY_VALUE)
		OnKeyValue();
	else if (msg.type == CATCHUP_COMMIT)
		OnCommit();
	else
		ASSERT_FAIL();
}

void CatchupReader::OnKeyValue()
{
//	Log_Trace();

	table->Set(&transaction, msg.key, msg.value);
}

void CatchupReader::OnCommit()
{
	Log_Trace();

	ReplicatedLog::Get()->SetPaxosID(&transaction, msg.paxosID);
	
	transaction.Commit();
	
	keyspaceDB->OnCatchupComplete();
	
	Close();
	
	ReplicatedLog::Get()->ContinuePaxos();
}
