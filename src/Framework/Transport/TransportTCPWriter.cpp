#include "TransportTCPWriter.h"

#define CONNECT_TIMEOUT		3
#define RECONNECT_TIMEOUT	3

TransportTCPWriter::TransportTCPWriter() :
onConnect(this, &TransportTCPWriter::OnConnect),
onConnectTimeout(this, &TransportTCPWriter::OnConnectTimeout),
connectTimeout(CONNECT_TIMEOUT, &onConnectTimeout)
{
	state = DISCONNECTED;
}

void TransportTCPWriter::Init(IOProcessor* ioproc_, Scheduler* scheduler_, Endpoint &endpoint_)
{
	TCPConn<MAX_TCP_MESSAGE_SIZE>::Init(ioproc_, false);
	scheduler = scheduler_;
	endpoint = endpoint_;
}

void TransportTCPWriter::Write(ByteString &bs)
{
	char lbuf[20];
	int llen;
	
	if (state == CONNECTED)
	{
		llen = snprintf(lbuf, sizeof(lbuf), "%d:", bs.length);
	
		TCPConn<MAX_TCP_MESSAGE_SIZE>::Write(lbuf, llen, false);		
		TCPConn<MAX_TCP_MESSAGE_SIZE>::Write(bs.buffer, bs.length);
	}
	else if (state == DISCONNECTED)
		Connect();
}

void TransportTCPWriter::Connect()
{
	bool ret;
	
	state = CONNECTING;

	socket.Create(TCP);
	socket.SetNonblocking();
	ret = socket.Connect(endpoint);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onConnect;
	tcpwrite.data.length = 0;
	
	ioproc->Add(&tcpwrite);
	scheduler->Reset(&connectTimeout);
}

void TransportTCPWriter::OnConnect()
{
	Log_Trace();
	
	state = CONNECTED;
	tcpwrite.onComplete = &onWrite;
	
	scheduler->Remove(&connectTimeout);
}

void TransportTCPWriter::OnConnectTimeout()
{
	OnClose();
}

void TransportTCPWriter::OnRead()
{
	Log_Message("should not read here");
}

void TransportTCPWriter::OnClose()
{
	Close();
	Connect();
}
