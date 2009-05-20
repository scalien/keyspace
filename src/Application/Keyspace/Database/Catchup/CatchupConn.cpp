#include "CatchupConn.h"
#include <math.h>
#include "Framework/Transport/Transport.h"

CatchupConn::CatchupConn(CatchupServer* server_)
{
	server = server_;
}

void CatchupConn::Init()
{
	TCPConn<PAXOS_BUF_SIZE>::Init(false);
	
	if (ReplicatedLog::Get()->GetTransaction()->IsActive())
		ReplicatedLog::Get()->GetTransaction()->Commit();
		
	// commit the current transaction so we see the latest state
	// or we could use get @@paxosID instead of ReplicatedLog::Get()->GetPaxosID()
	
	transaction.Set(server->table);
	transaction.Begin();
	server->table->Iterate(&transaction, cursor);
	tcpwrite.data = writeBuffer;
	paxosID = ReplicatedLog::Get()->GetPaxosID();
	
	WriteNext();
}

void CatchupConn::OnRead()
{
}

void CatchupConn::OnWrite()
{
	Log_Trace();
	
	if (msg.type == CATCHUP_COMMIT)
	{
		cursor.Close();
		if (transaction.IsActive())
			transaction.Rollback();
		Close();
		delete this;
	}
	else
		WriteNext();
}

void CatchupConn::OnClose()
{
	Log_Trace();

	cursor.Close();
	if (transaction.IsActive())
		transaction.Rollback();
	Close();
	delete this;
}

void CatchupConn::WriteNext()
{
	Log_Trace();

	static ByteArray<32> prefix; // for the "length:" header
	static ByteArray<MAX_TCP_MESSAGE_SIZE> msgData;
	bool kv;
	
	kv = false;
	while(true)
	{
		kv = cursor.Next(key, value);
		
		if (!kv)
		{
			Log_Trace("Sending commit!");
			msg.Commit(paxosID);
			break;
		}
		
		if (key.length > 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
			continue;
		else
		{
			msg.KeyValue(key, value);
			break;
		}
	}
	
	msg.Write(msgData);
	
	// prepend with the "length:" header
	tcpwrite.data.length = snwritef(tcpwrite.data.buffer, tcpwrite.data.size, "%d:%B",
		msgData.length, msgData.length, msgData.buffer);
	tcpwrite.transferred = 0;

	IOProcessor::Add(&tcpwrite);
}
