#include "CatchupReader.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "Application/Keyspace/Database/ReplicatedKeyspaceDB.h"

void CatchupReader::Init(ReplicatedKeyspaceDB* keyspaceDB_, Table* table_)
{
	keyspaceDB = keyspaceDB_;
	table = table_;
	
	transaction.Set(table);
}

void CatchupReader::Shutdown()
{
	if (state != DISCONNECTED)
		OnClose();
}

void CatchupReader::Start(unsigned nodeID)
{
	Log_Trace();

	bool ret;
	Endpoint endpoint;

	ret = true;
	count = 0;
	ret &= transaction.Begin();
	if (!ret)
		ASSERT_FAIL();

	endpoint = RCONF->GetEndpoint(nodeID);
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
	count++;
	if (count % CATCHUP_COMMIT_GRANULARITY == 0)
	{
		transaction.Commit();
		transaction.Begin();
	}
}

void CatchupReader::OnCommit()
{
	Log_Trace();

	RLOG->SetPaxosID(&transaction, msg.paxosID);
	
	transaction.Commit();
	
	keyspaceDB->OnCatchupComplete();
	
	Close();
}
