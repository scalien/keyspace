#include "CatchupClient.h"
#include "Framework/ReplicatedLog/ReplicatedConfig.h"
#include "Application/Keyspace/Database/ReplicatedKeyspaceDB.h"

void CatchupClient::Init(ReplicatedKeyspaceDB* keyspaceDB_, Table* table_)
{
	keyspaceDB = keyspaceDB_;
	table = table_;
	
	transaction.Set(table);
}

void CatchupClient::Start(unsigned nodeID)
{
	Log_Trace();

	bool ret;
	Endpoint endpoint;
	
	ret = true;
	ret &= transaction.Begin();
	ret &= table->Truncate(&transaction);
	if (!ret)
		ASSERT_FAIL();

	endpoint = ReplicatedConfig::Get()->endpoints[nodeID];
	endpoint.SetPort(endpoint.GetPort() + CATCHUP_PORT_OFFSET);
	Connect(endpoint, CATCHUP_CONNECT_TIMEOUT);
}

void CatchupClient::OnMessageRead(const ByteString& message)
{
	if (msg.Read(message))
		ProcessMsg();
}

void CatchupClient::OnClose()
{
	Log_Trace();

	if (state == DISCONNECTED)
		return;

	if (transaction.IsActive())
		transaction.Rollback();
	
	keyspaceDB->OnCatchupFailed();
	
	Close();
}

void CatchupClient::OnConnect()
{
	Log_Trace();

	TCPConn<>::OnConnect();
	
	AsyncRead();
}

void CatchupClient::OnConnectTimeout()
{
	Log_Trace();

	OnClose();
}

void CatchupClient::ProcessMsg()
{
	Log_Trace();

	if (msg.type == CATCHUP_KEY_VALUE)
		OnKeyValue();
	else if (msg.type == CATCHUP_COMMIT)
		OnCommit();
	else
		ASSERT_FAIL();
}

void CatchupClient::OnKeyValue()
{
	Log_Trace();

	table->Set(&transaction, msg.key, msg.value);
}

void CatchupClient::OnCommit()
{
	Log_Trace();

	ReplicatedLog::Get()->SetPaxosID(&transaction, msg.paxosID);
	
	transaction.Commit();
	
	keyspaceDB->OnCatchupComplete();
	
	Close();
}
