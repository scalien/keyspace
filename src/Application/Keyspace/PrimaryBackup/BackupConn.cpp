#include "BackupConn.h"
#include "BackupServer.h"

void BackupConn::Init(BackupServer* server_)
{
	server = server_;
	state = HANDSHAKE;
	closeAfterSend = false;
	
	TCPConn<>::Init();
	
	Log_Trace();
}

void BackupConn::OnMessageRead(const ByteString& msg)
{
	char		type;
	uint64_t	cmdID;
	ByteString	uuid;
	int			read;
	
	if (state == HANDSHAKE)
	{
		read = snreadf(msg.buffer, msg.length, "%c:%U:%N",
					   &type, &cmdID, &uuid);
		
		if (read != (int) msg.length)
		{
			WriteError(PROTOCOL);
			return;
		}
		
		if (/*uuid != server->GetUUID()*/ false)
		{
			WriteError(UUID);
			return;
		}
		
		if (cmdID < server->GetLastKnownID())
			state = DUMP;
		else if (cmdID < server->GetLastID())
			state = CATCHUP;
		else if (cmdID == server->GetLastID())
		{
			state = SYNC;
			server->RegisterBackupConn(this);
		}
		else
		{
			Log_Trace("%lu", (unsigned) cmdID);
			WriteError(CMDID);
			return;
		}
	}
	
	Log_Trace();
}

void BackupConn::OnClose()
{
	Close();
	server->DeleteConn(this);
}

void BackupConn::OnWrite()
{
	TCPConn<>::OnWrite();
	if (closeAfterSend && !tcpwrite.active)
		OnClose();
}

void BackupConn::WriteKeyspaceMsg(const KeyspaceMsg& msg)
{
	ByteString*	buffer;
	
	// TODO make this safe!
	buffer = EnsureBuffer(readBuffer.size);
	msg.Write(*buffer);
	WritePending();
}

void BackupConn::WriteError(Error e)
{
	closeAfterSend = true;
	
	switch (e)
	{
	case PROTOCOL:
		Write("p", 1);
		break;
	case UUID:
		Write("u", 1);
		break;
	case CMDID:
		Write("c", 1);
		break;
	case RATE:
		Write("r", 1);
		break;
	}
}

