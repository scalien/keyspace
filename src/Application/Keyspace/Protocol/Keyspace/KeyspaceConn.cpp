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
	KeyspaceClient::Init(kdb_);
	
	server = server_;
	closeAfterSend = false;
	
	AsyncRead();
}

void KeyspaceConn::OnComplete(KeyspaceOp* op, bool status, bool final)
{
	Log_Trace();
	
	if (op->type == KeyspaceOp::GET ||
		op->type == KeyspaceOp::DIRTY_GET ||
		op->type == KeyspaceOp::ADD)
	{
		if (status)
			resp.Ok(op->value);
		else
			resp.NotFound();
	}
	else if (op->type == KeyspaceOp::SET ||
			 op->type == KeyspaceOp::TEST_AND_SET)
	{
		if (status)
			resp.Ok();
		else
			resp.Failed();
	}
	else if (op->type == KeyspaceOp::DELETE)
	{
		if (status)
			resp.Ok();
		else
			resp.NotFound();
	}
	else if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
	{
		Write(op->key);
		if (final)
			resp.ListEnd();
	}
	else
		ASSERT_FAIL();
	
	resp.Write(data);
	Write(data);
	
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
		msglength = strntouint64_t(tcpread.data.buffer, tcpread.data.length, &nread);
		
		if (nread == 0 || tcpread.data.length <= nread)
			break;
			
		if (tcpread.data.buffer[nread] != ':')
			ASSERT_FAIL();
	
		msgbegin = nread + 1;
		msgend = nread + 1 + msglength;

		if (tcpread.data.length < msgend)
		{
			// request just the right amount
			tcpread.requested = msgend - tcpread.data.length;
			break;
		}

		if (req.Read(ByteString(msglength, msglength, tcpread.data.buffer + msgbegin)))
			ProcessMsg();
		else
			OnClose();
		
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

	TCPConn<>::Write(prefix.buffer, prefix.length);
	TCPConn<>::Write(bs.buffer, bs.length);
}

void KeyspaceConn::ProcessMsg()
{
	Log_Trace();
	
	KeyspaceOp* op;
	
	if (req.type == KEYSPACECLIENT_GETMASTER)
	{
		return;
	}
	else if (req.type == KEYSPACECLIENT_SUBMIT)
	{
		kdb->Submit();
		return;
	}
	
	op = new KeyspaceOp;
	op->client = this;
	
	if (!req.ToKeyspaceOp(op))
	{
		delete op;
		OnClose();
		return;
	}
	
	if (!Add(op, false))
	{
		delete op;
		resp.Failed();
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
		Close();
}
