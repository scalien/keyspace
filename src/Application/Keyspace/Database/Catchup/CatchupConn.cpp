#include "CatchupConn.h"
#include <math.h>
#include "Transport.h"

CatchupConn::CatchupConn(CatchupServer* server_)
{
	TCPConn<BUF_SIZE>::Init(false /*startRead*/);
	
	server = server_;
	transaction.Set(server->table);
	transaction.Begin();
	server->table->Iterate(&transaction, cursor);
	tcpwrite.data = writeBuffer;
	startPaxosID = server->replicatedLog->GetPaxosID();
	
	WriteNext();
}

void CatchupConn::OnRead()
{
}

void CatchupConn::OnWrite()
{
	if (msg.type == CATCHUP_COMMIT)
	{
		transaction.Rollback(); // we just read from the DB, so we could Commit(), too
		Close();
		delete this;
	}
	else
		WriteNext();
}

void CatchupConn::OnClose()
{
	Log_Trace();

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
	ByteString	key, value;
	bool kv;
	
	kv = false;
	while(true)
	{
		kv = cursor.Next(key, value);
		
		if (!kv)
		{
			msg.Commit(startPaxosID);
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
	snprintf(writeBuffer.buffer, writeBuffer.size, "%d:%.*s",
		msgData.length, msgData.length, msgData.buffer);
	
	IOProcessor::Get()->Add(&tcpwrite);
}
