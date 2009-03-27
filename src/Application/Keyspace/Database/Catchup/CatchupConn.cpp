#include "CatchupConn.h"
#include <math.h>

CatchupConn::CatchupConn(CatchupServer* server_)
{
	TCPConn<BUF_SIZE>::Init(IOProcessor::Get(), false /*startRead*/);
	
	server = server_;	
	server->table->Iterate(cursor);
	tcpwrite.data = writeBuffer;
	startPaxosID = server->replicatedLog->GetPaxosID();
	
	WriteNext();
}

void CatchupConn::OnRead()
{
}

void CatchupConn::OnWrite()
{
	if (msg.type == CATCHUP_COMMIT || msg.type == CATCHUP_ROLLBACK)
	{
		Close();
		delete this;
	}
}

void CatchupConn::OnClose()
{
	Close();
	delete this;
}

void CatchupConn::WriteNext()
{
	int			msglen;
	ByteString	key, value;
	
	if (msg.type == KEY_VALUE)
	{
		if (cursor.Next(key, value))
		{
			msg.KeyValue(key, value);
			msglen = NumLen(key.length) + NumLen(value.length) + 3 + key.length + value.length;
		}
		else
		{
			endPaxosID = server->replicatedLog->GetPaxosID();			
			if (server->replicatedLog->GetLogItem(startPaxosID, value))
				msg.DBCommand(startPaxosID, value);
			else
				msg.Rollback();
		}
	}
	else if (msg.type == DB_COMMAND && msg.paxosID < endPaxosID)
	{
		if (server->replicatedLog->GetLogItem(msg.paxosID + 1, value))
			msg.DBCommand(msg.paxosID + 1, value);			
		else
			msg.Rollback();
	}
	else // (msg.type == DB_COMMAND && msg.paxosID == endPaxosID)
	{
		msg.Commit();
	}
	
	writeBuffer.length = snprintf(writeBuffer.buffer, writeBuffer.size, "%d:", msglen);
	
	msg.Write(writeBuffer);
	
	ioproc->Add(&tcpwrite);
}
