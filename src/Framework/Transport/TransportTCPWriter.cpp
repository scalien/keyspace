#include "TransportTCPWriter.h"

#define CONNECT_TIMEOUT		2000

TransportTCPWriter::TransportTCPWriter() :
onConnect(this, &TransportTCPWriter::OnConnect),
onConnectTimeout(this, &TransportTCPWriter::OnConnectTimeout),
connectTimeout(CONNECT_TIMEOUT, &onConnectTimeout)
{
	state = DISCONNECTED;
}

TransportTCPWriter::~TransportTCPWriter()
{
}

void TransportTCPWriter::Init(IOProcessor* ioproc_, Scheduler* scheduler_, Endpoint &endpoint_)
{
	TCPConn<MAX_TCP_MESSAGE_SIZE>::Init(ioproc_, false);
	scheduler = scheduler_;
	endpoint = endpoint_;
	Connect();

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
	else if (state == DISCONNECTED && !connectTimeout.IsActive())
		Connect();
}

void TransportTCPWriter::Connect()
{
	Log_Message("endpoint = %s", endpoint.ToString());
	
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
	Log_Message("endpoint = %s", endpoint.ToString());
	
	state = CONNECTED;
	tcpwrite.onComplete = &onWrite;
	
	scheduler->Remove(&connectTimeout);
}

void TransportTCPWriter::OnConnectTimeout()
{
	Log_Message("endpoint = %s", endpoint.ToString());
	
	Close();
	Connect();
}

void TransportTCPWriter::OnRead()
{
	ASSERT_FAIL();
}

void TransportTCPWriter::OnClose()
{
	Log_Message("endpoint = %s", endpoint.ToString());
	
	if (!connectTimeout.IsActive())
		scheduler->Add(&connectTimeout);
}
