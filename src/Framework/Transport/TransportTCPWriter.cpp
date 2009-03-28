#include "TransportTCPWriter.h"
#include "System/Events/EventLoop.h"

#define CONNECT_TIMEOUT		2000


void TransportTCPWriter::Init(Endpoint &endpoint_)
{
	endpoint = endpoint_;
	TCPConn<MAX_TCP_MESSAGE_SIZE>::Connect(endpoint, CONNECT_TIMEOUT);
}

void TransportTCPWriter::Write(ByteString &bs)
{
	Log_Trace();

	char lbuf[20];
	int llen;
	
	if (state == CONNECTED)
	{
		llen = snprintf(lbuf, sizeof(lbuf), "%d:", bs.length);
	
		TCPConn<MAX_TCP_MESSAGE_SIZE>::Write(lbuf, llen, false);		
		TCPConn<MAX_TCP_MESSAGE_SIZE>::Write(bs.buffer, bs.length);
	}
	else if (state == DISCONNECTED && !connectTimeout.IsActive())
		Connect();
}

void TransportTCPWriter::Connect()
{
	Log_Trace();
	
	TCPConn<MAX_TCP_MESSAGE_SIZE>::Connect(endpoint, CONNECT_TIMEOUT);
}

void TransportTCPWriter::OnConnect()
{
	Log_Message("endpoint = %s", endpoint.ToString());
	tcpwrite.onComplete = &onWrite;
	
	AsyncRead();
}

void TransportTCPWriter::OnConnectTimeout()
{
	Log_Message("endpoint = %s", endpoint.ToString());
	
	Close();
	Connect();
}

void TransportTCPWriter::OnRead()
{
	Log_Message("endpoint = %s", endpoint.ToString());
	
	// drop any data
	readBuffer.Clear();
	AsyncRead();
}

void TransportTCPWriter::OnClose()
{
	Log_Message("endpoint = %s", endpoint.ToString());
	
	if (!connectTimeout.IsActive())
		EventLoop::Get()->Reset(&connectTimeout);
}
