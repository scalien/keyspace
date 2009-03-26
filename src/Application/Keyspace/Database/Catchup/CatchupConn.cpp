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
	LogItem*	item;
	
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
			
			item = server->replicatedLog->GetLogItem(startPaxosID);
			if (item == NULL)
				msg.Rollback();
			else
				msg.DBCommand(item->paxosID, item->value);			
		}
	}
	else if (msg.type == DB_COMMAND && msg.paxosID < endPaxosID)
	{
		item = server->replicatedLog->GetLogItem(msg.paxosID + 1);
		if (item == NULL)
			msg.Rollback();
		else
			msg.DBCommand(item->paxosID, item->value);
	}
	else // (msg.type == DB_COMMAND && msg.paxosID == endPaxosID)
	{
		msg.Commit();
	}
	
	writeBuffer.length = snprintf(writeBuffer.buffer, writeBuffer.size, "%d:", msglen);
	
	msg.Write(writeBuffer);
	
	ioproc->Add(&tcpwrite);
}
