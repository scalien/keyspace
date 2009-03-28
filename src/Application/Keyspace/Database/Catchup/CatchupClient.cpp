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
	Endpoint endpoint;
	
	transaction.Begin();
	table->Drop();

	endpoint = PaxosConfig::Get()->endpoints[nodeID];
	endpoint.SetPort(endpoint.GetPort() + CATCHUP_PORT_OFFSET);
}

void CatchupClient::OnRead()
{
	int msglength, nread, msgbegin, msgend;
	Endpoint endpoint;
	
	tcpread.requested = IO_READ_ANY;
	
	do
	{
		msglength = strntol(tcpread.data.buffer, tcpread.data.length, &nread);
		
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
	
	IOProcessor::Get()->Add(&tcpread);

}

void CatchupClient::OnClose()
{
	if (transaction.IsActive())
		transaction.Rollback();
	
	keyspaceDB->OnCatchupFailed();
	
	Close();
}

void CatchupClient::ProcessMsg()
{
	if (msg.type == KEY_VALUE)
		OnKeyValue();
	else if (msg.type == CATCHUP_COMMIT)
		OnCommit();
	else
		ASSERT_FAIL();
}

void CatchupClient::OnKeyValue()
{
	table->Set(&transaction, msg.key, msg.value);
}

void CatchupClient::OnCommit()
{
	// TODO: set paxos stuff
	//ReplicatedLog::Get()->SetPaxos(msg.paxosID);
	
	transaction.Commit();
	
	keyspaceDB->OnCatchupComplete();
	
	Close();
}
