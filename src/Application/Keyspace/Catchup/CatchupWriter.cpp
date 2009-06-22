#include "CatchupWriter.h"
#include <math.h>
#include "CatchupServer.h"
#include "Framework/Transport/Transport.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"

CatchupWriter::CatchupWriter()
{
}

void CatchupWriter::Init(CatchupServer* server_)
{
	Log_Trace();

	TCPConn<>::Init(true);
	
	server = server_;
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

void CatchupWriter::OnRead()
{
	AsyncRead();
}

void CatchupWriter::OnWrite()
{
//	Log_Trace();
	
	if (msg.type == CATCHUP_COMMIT)
	{
		cursor.Close();
		if (transaction.IsActive())
			transaction.Rollback();
		Close();
		server->DeleteConn(this);
	}
	else
		WriteNext();
}

void CatchupWriter::OnClose()
{
	Log_Trace();

	cursor.Close();
	if (transaction.IsActive())
		transaction.Rollback();
	Close();
	server->DeleteConn(this);
}

void CatchupWriter::WriteNext()
{
//	Log_Trace();

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
		&msgData);
	tcpwrite.transferred = 0;

	IOProcessor::Add(&tcpwrite);
}
