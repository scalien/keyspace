#include "KeyspaceConn.h"
#include "KeyspaceServer.h"

KeyspaceConn::KeyspaceConn()
{
	server = NULL;
}

void KeyspaceConn::Init(KeyspaceDB* kdb_, KeyspaceServer* server_)
{
	Log_Trace();
	
	TCPConn<>::Init();
	KeyspaceService::Init(kdb_);
	
	server = server_;
	closeAfterSend = false;
	
	AsyncRead();
}

void KeyspaceConn::OnComplete(KeyspaceOp* op, bool status, bool final)
{
	Log_Trace();
	
	data.Clear();
	
	if (!status && final && op->MasterOnly() && !kdb->IsMaster())
	{
		resp.NotMaster(op->cmdID);
		resp.Write(data);
	}
	else
	{
		if (op->type == KeyspaceOp::GET ||
			op->type == KeyspaceOp::DIRTY_GET ||
			op->type == KeyspaceOp::ADD)
		{
			if (status)
				resp.Ok(op->cmdID, op->value);
			else
				resp.Failed(op->cmdID);

			resp.Write(data);
		}
		else if (op->type == KeyspaceOp::SET ||
				 op->type == KeyspaceOp::TEST_AND_SET)
		{
			if (status)
				resp.Ok(op->cmdID);
			else
				resp.Failed(op->cmdID);

			resp.Write(data);
		}
		else if (op->type == KeyspaceOp::DELETE)
		{
			if (status)
				resp.Ok(op->cmdID);
			else
				resp.Failed(op->cmdID);

			resp.Write(data);
		}
		else if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
		{
			if (op->key.length > 0)
			{
				resp.ListItem(op->cmdID, op->key);
				resp.Write(data);
			}
		}
		else if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
		{
			if (op->key.length > 0)
			{
				resp.ListPItem(op->cmdID, op->key, op->value);
				resp.Write(data);
			}
		}
		else
			ASSERT_FAIL();
		
		if (data.length > 0)
		{
			Log_Message("=== Sending to client: %.*s ===", data.length, data.buffer);
			Write(data);
		}
		
		if (final && 
			(op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST ||
			 op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP))
		{
			resp.ListEnd(op->cmdID);
			resp.Write(data);
			Log_Message("=== Sending to client: %.*s ===", data.length, data.buffer);
			Write(data);
		}
	}
	
	if (final)
	{
		numpending--;
		delete op;
	}

	if (state == DISCONNECTED && numpending == 0)
		server->DeleteConn(this);
}

void KeyspaceConn::OnRead()
{
	Log_Trace();

	unsigned msglength, nread, msgbegin, msgend;
	
	tcpread.requested = IO_READ_ANY;
	
	do
	{
		Log_Message("tcpread buffer %.*s", tcpread.data.length, tcpread.data.buffer);
		msglength = strntouint64_t(tcpread.data.buffer, tcpread.data.length, &nread);
		
		if (msglength > (unsigned) (tcpread.data.size - 8) || nread > 7) // largest prefix: 100xxxx:
		{
			OnClose();
			return;
		}
		
		if (nread == 0 || (unsigned) tcpread.data.length <= nread)
			break;
			
		if (tcpread.data.buffer[nread] != ':')
			ASSERT_FAIL();
	
		msgbegin = nread + 1;
		msgend = nread + 1 + msglength;
		
		Log_Message("%d %d %d", msgbegin, msgend, tcpread.data.length);

		if ((unsigned) tcpread.data.length < msgend)
		{
			tcpread.requested = msgend;
			break;
		}

		Log_Message("Attempting Read() with %.*s", msglength, tcpread.data.buffer + msgbegin);
		if (req.Read(ByteString(msglength, msglength, tcpread.data.buffer + msgbegin)))
			ProcessMsg();
		else
		{
			OnClose();
			return;
		}
		
		// move the rest back to the beginning of the buffer
		memcpy(tcpread.data.buffer, tcpread.data.buffer + msgend,
			tcpread.data.length - msgend);
		tcpread.data.length = tcpread.data.length - msgend;
		
		if (tcpread.data.length == 0)
			break;
	}
	while (true);
	
	if (state == CONNECTED)
		IOProcessor::Add(&tcpread);
}

void KeyspaceConn::Write(ByteString &bs)
{
	static ByteArray<64> prefix;
	
	prefix.length = snprintf(prefix.buffer, prefix.size, "%d:", bs.length);

	TCPConn<>::Write(prefix.buffer, prefix.length, false);
	TCPConn<>::Write(bs.buffer, bs.length);
}

void KeyspaceConn::ProcessMsg()
{
	Log_Trace();

	static ByteArray<32> ba;
	
	if (req.type == KEYSPACECLIENT_GETMASTER)
	{
		int master = kdb->GetMaster();
		if (master < 0)
			resp.Failed(req.cmdID);
		else
		{
			ba.length = snprintf(ba.buffer, ba.size, "%d", master);
			resp.Ok(req.cmdID, ba);
		}
		resp.Write(data);
		Write(data);
		return;
	}
	else if (req.type == KEYSPACECLIENT_SUBMIT)
	{
		kdb->Submit();
		return;
	}
	
	KeyspaceOp* op;
	
	op = new KeyspaceOp;
	op->service = this;
	
	if (!req.ToKeyspaceOp(op))
	{
		delete op;
		OnClose();
		return;
	}
	
	if (!Add(op, false))
	{
		delete op;
		resp.Failed(op->cmdID);
		resp.Write(data);
		Write(data);
		closeAfterSend = true;
		return;
	}
}

void KeyspaceConn::OnClose()
{
	Log_Trace();
	
	Close();
	if (numpending == 0)
		server->DeleteConn(this);
}


void KeyspaceConn::OnWrite()
{
	Log_Trace();
	
	TCPConn<>::OnWrite();
	if (closeAfterSend && !tcpwrite.active)
		OnClose();
}
