#include "CatchupConn.h"
#include <math.h>
#include "Transport.h"

CatchupConn::CatchupConn(CatchupServer* server_)
{
	TCPConn<BUF_SIZE>::Init(IOProcessor::Get(), false /*startRead*/);
	
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
	if (transaction.IsActive())
		transaction.Rollback();
	Close();
	delete this;
}

void CatchupConn::WriteNext()
{
	static ByteArray<32> prefix; // for the "length:" header
	static ByteArray<MAX_TCP_MESSAGE_SIZE> msgData;
	ByteString	key, value;
	
	if (cursor.Next(key, value))
		msg.KeyValue(key, value);
	else
		msg.Commit(startPaxosID);
	
	msg.Write(msgData);
	
	// prepend with the "length:" header
	snprintf(writeBuffer.buffer, writeBuffer.size, "%d:%.*s",
		msgData.length, msgData.length, msgData.buffer);
	
	ioproc->Add(&tcpwrite);
}
