#include "CatchupClient.h"
#include "Framework/Paxos/PaxosConfig.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"

void CatchupClient::Init(KeyspaceDB* keyspaceDB_, Table* table_)
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

	endpoint = PaxosConfig::Get()->endpoints[nodeID];
	endpoint.SetPort(endpoint.GetPort() + CATCHUP_PORT_OFFSET);
	Connect(endpoint, CATCHUP_CONNECT_TIMEOUT);
}

void CatchupClient::OnRead()
{
	Log_Trace();

	int msglength, nread, msgbegin, msgend;
	Endpoint endpoint;
	
	tcpread.requested = IO_READ_ANY;
	
	do
	{
		msglength = strntolong64(tcpread.data.buffer, tcpread.data.length, &nread);
		
		if (nread == 0 || tcpread.data.length <= nread)
			break;
			
		if (tcpread.data.buffer[nread] != ':')
			ASSERT_FAIL();
	
		msgbegin = nread + 1;
		msgend = nread + 1 + msglength;

		if (tcpread.data.length < msgend)
		{
			// request just the right amount
			tcpread.requested = msgend - tcpread.data.length;
			break;
		}

		if (msg.Read(ByteString(msglength, msglength, tcpread.data.buffer + msgbegin)))
			ProcessMsg();
		
		// move the rest back to the beginning of the buffer
		memcpy(tcpread.data.buffer, tcpread.data.buffer + msgend,
			tcpread.data.length - msgend);
		tcpread.data.length = tcpread.data.length - msgend;
		
		if (tcpread.data.length == 0)
			break;
	}
	while (true);
	
	if (state == CONNECTED)
		IOProcessor::Add(&tcpread);
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

	if (msg.type == KEY_VALUE)
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
