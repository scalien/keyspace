#include "TransportTCPWriter.h"
#include "System/Events/EventLoop.h"

#define CONNECT_TIMEOUT		2000


bool TransportTCPWriter::Init(Endpoint &endpoint_)
{
	endpoint = endpoint_;
	TCPConn<>::Connect(endpoint, CONNECT_TIMEOUT);
	return true;
}

void TransportTCPWriter::Write(ByteString &bs)
{
	Log_Trace();

	char lbuf[20];
	int llen;
	
	if (state == CONNECTED || state == CONNECTING)
	{
		llen = snwritef(lbuf, sizeof(lbuf), "%d:", bs.length);
		
		TCPConn<>::Write(lbuf, llen, false);		
		TCPConn<>::Write(bs.buffer, bs.length);
	}
	else if (state == DISCONNECTED && !connectTimeout.IsActive())
		Connect();
}

void TransportTCPWriter::Connect()
{
	Log_Trace();
	
	TCPConn<>::Connect(endpoint, CONNECT_TIMEOUT);
}

void TransportTCPWriter::OnConnect()
{
	TCPConn<>::OnConnect();
	
	Log_Trace("endpoint = %s", endpoint.ToString());
	
	AsyncRead();
}

void TransportTCPWriter::OnConnectTimeout()
{
	TCPConn<>::OnConnectTimeout();
	
	Log_Trace("endpoint = %s", endpoint.ToString());
	
	Close();
	Connect();
}

void TransportTCPWriter::OnRead()
{
	Log_Trace("endpoint = %s", endpoint.ToString());
	
	// drop any data
	readBuffer.Clear();
	AsyncRead();
}

void TransportTCPWriter::OnClose()
{
	Log_Trace("endpoint = %s", endpoint.ToString());
	
	if (!connectTimeout.IsActive())
	{
		Log_Trace("reset");
		EventLoop::Reset(&connectTimeout);
	}
}
