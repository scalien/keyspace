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
	
//	RLOG->StopPaxos();

	server = server_;
	table = database.GetTable("keyspace");
	transaction.Set(table);
	transaction.Begin();
	table->Iterate(&transaction, cursor);
	tcpwrite.data.Set(writeBuffer);

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
	if (msg.type == CATCHUP_COMMIT)
		OnClose();
	else
		WriteNext();
}

void CatchupWriter::OnClose()
{
	Log_Trace();

//	RLOG->ContinuePaxos();

	cursor.Close();
	if (transaction.IsActive())
		transaction.Rollback();
	Close();
	server->DeleteConn(this);
}

void CatchupWriter::WriteNext()
{
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
		{
			if (strncmp(key.buffer, "@@round:", MIN(key.length, sizeof("@@round:") - 1)) == 0)
			{
				msg.KeyValue(key, value);
				break;

			}
			continue;
		}
		else
		{
			msg.KeyValue(key, value);
			break;
		}
	}
	
	msg.Write(msgData);
	
	// prepend with the "length:" header
	tcpwrite.data.length = snwritef(tcpwrite.data.buffer, tcpwrite.data.size,
									"%M", &msgData);
	tcpwrite.transferred = 0;

	IOProcessor::Add(&tcpwrite);
}
