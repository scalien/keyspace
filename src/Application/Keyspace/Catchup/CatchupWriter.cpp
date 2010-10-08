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

	if (!table->Get(NULL, "@@paxosID", paxosID))
		ASSERT_FAIL();
	
    key.Clear();
	WriteNext();
}

void CatchupWriter::OnRead()
{
	AsyncRead();
}

void CatchupWriter::OnWrite()
{
	TCPConn<>::OnWrite();

	if (msg.type == CATCHUP_COMMIT)
	{
		if (BytesQueued() == 0)
			OnClose();
	}
	else
	{
		if (BytesQueued() < MAX_TCP_MESSAGE_SIZE)
			WriteNext();
	}
}

void CatchupWriter::OnClose()
{
	Log_Trace();

	Close();
	server->DeleteConn(this);
}

void CatchupWriter::WriteNext()
{
	bool    kv;
    bool    first;

	table->Iterate(NULL, cursor);
    first = (key.length == 0);
    kv = cursor.Start(key, value);
	while(true)
	{
        if (!first)
            kv = cursor.Next(key, value);
        else
            first = false;
		if (!kv)
		{
			Log_Trace("Sending commit!");
			msg.Commit(paxosID);
		}
		else
		{
			if (key.length > 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
			{
				if (strncmp(key.buffer, "@@pround:", MIN(key.length, sizeof("@@pround:") - 1)) == 0)
					msg.KeyValue(key, value);
				else
					continue;
			}
			// but write keys starting with "!!"
			else
				msg.KeyValue(key, value);
		}

		msg.Write(msgData);
		writeBuffer.Writef("%M", &msgData);
		Write(writeBuffer.buffer, writeBuffer.length, false);
		if (BytesQueued() < MAX_TCP_MESSAGE_SIZE && msg.type != CATCHUP_COMMIT)
			continue;
		else
			break;
	}
    cursor.Close();
	WritePending();
}
