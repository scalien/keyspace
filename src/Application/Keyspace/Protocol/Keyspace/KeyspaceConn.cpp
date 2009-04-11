#include "KeyspaceConn.h"
#include "KeyspaceServer.h"

KeyspaceConn::KeyspaceConn()
{
	server = NULL;
}


void KeyspaceConn::Init(KeyspaceDB* kdb_, KeyspaceServer* server_)
{
	TCPConn<>::Init();
	KeyspaceClient::Init(kdb_);
	
	server = server_;
	closeAfterSend = false;
	
	AsyncRead();
}

void KeyspaceConn::OnComplete(KeyspaceOp* op, bool status, bool final)
{
	if (final)
	{
		numpending--;
		delete op;
		closeAfterSend = true;
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

void KeyspaceConn::ProcessMsg()
{
	if (msg.type == KEYSPACECLIENT_SUBMIT)
	{
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
