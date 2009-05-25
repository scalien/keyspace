#include "CatchupConn.h"
#include <math.h>
#include "Framework/Transport/Transport.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"

CatchupConn::CatchupConn()
{
}

void CatchupConn::Init()
{
	TCPConn<PAXOS_BUF_SIZE>::Init(false);
	
	table = database.GetTable("keyspace");
	transaction.Set(table);
	transaction.Begin();
	table->Iterate(&transaction, cursor);
	tcpwrite.data = writeBuffer;

	// this is not good, I'm not seeing the currently active transaction
	if (!table->Get(&transaction, "@@paxosID", paxosID))
		ASSERT_FAIL();
	
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
	tcpwrite.data.length = snwritef(tcpwrite.data.buffer, tcpwrite.data.size, "%M",
		msgData.length, msgData.buffer);
	tcpwrite.transferred = 0;

	IOProcessor::Add(&tcpwrite);
}
