#include "KeyspaceConn.h"
#include "KeyspaceServer.h"

#define MSG_OK				"o"
#define MSG_NOTFOUND		"n"
#define MSG_FAILED			"f"
#define MSG_LISTEND			"*"
#define RESPONSE_OK			TCPConn<>::Write(MSG_OK, strlen(MSG_OK))
#define RESPONSE_NOTFOUND	TCPConn<>::Write(MSG_NOTFOUND, strlen(MSG_NOTFOUND))
#define RESPONSE_FAILED		TCPConn<>::Write(MSG_FAILED, strlen(MSG_FAILED))
#define RESPONSE_LISTEND	TCPConn<>::Write(MSG_LISTEND, strlen(MSG_LISTEND))

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
			Write(op->value);
		else
			RESPONSE_NOTFOUND;
	}
	else if (op->type == KeyspaceOp::SET ||
			 op->type == KeyspaceOp::TEST_AND_SET)
	{
		if (status)
			RESPONSE_OK;
		else
			RESPONSE_FAILED;
	}
	else if (op->type == KeyspaceOp::DELETE)
	{
		if (status)
			RESPONSE_OK;
		else
			RESPONSE_NOTFOUND;
	}
	else if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
	{
		Write(op->key);
		if (final)
			RESPONSE_LISTEND;
	}
	else
		ASSERT_FAIL();
	
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

		if (msg.Read(ByteString(msglength, msglength, tcpread.data.buffer + msgbegin)))
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

	Write(prefix);
	TCPConn<>::Write(bs.buffer, bs.length, true);
}

void KeyspaceConn::ProcessMsg()
{
	Log_Trace();
	
	KeyspaceOp* op;
	
	if (msg.type == KEYSPACECLIENT_GETMASTER)
	{
	}
	else if (msg.type == KEYSPACECLIENT_SUBMIT)
		kdb->Submit();
	
	op = new KeyspaceOp;
	op->client = this;
	
	if (!msg.ToKeyspaceOp(op))
	{
		delete op;
		OnClose();
	}
	
	Add(op, false);
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

#undef MSG_OK
#undef MSG_NOTFOUND
#undef MSG_FAILED
#undef MSG_LISTEND
#undef RESPONSE_OK
#undef RESPONSE_NOTFOUND
#undef RESPONSE_FAILED
#undef RESPONSE_LISTEND
