#include "TransportTCPReader.h"

#include "System/Common.h"

/* class TransportTCPConn */

void TransportTCPConn::OnMessageRead(const ByteString& message)
{
	reader->SetMessage(message);
	
	if (!reader->stopped)
		Call(reader->onRead);
}
	
void TransportTCPConn::OnClose()
{
	Log_Trace();
	
	socket.Close();
	delete this;
}


/* class TransportTCPReader */

bool TransportTCPReader::Init(int port)
{
	onRead = NULL;
	stopped = false;
	return TCPServer::Init(port);
}

void TransportTCPReader::SetOnRead(Callable* onRead_)
{
	onRead = onRead_;
}

void TransportTCPReader::SetMessage(ByteString bs_)
{
	bs = bs_;
}

void TransportTCPReader::GetMessage(ByteString& bs_)
{
	bs_ = bs;
}

void TransportTCPReader::Stop()
{
	Log_Trace();
	
	stopped = true;
}

void TransportTCPReader::Continue()
{
	Log_Trace();
	
	stopped = false;
}

void TransportTCPReader::OnConnect()
{
	Log_Trace();
	
	TransportTCPConn* conn = new TransportTCPConn(this);
	
	if (listener.Accept(&(conn->GetSocket())))
	{
		Endpoint endpoint;
		conn->GetSocket().GetEndpoint(endpoint);
		
		Log_Message("%s connected, fd = %d", endpoint.ToString(), conn->GetSocket().fd);
		
		conn->GetSocket().SetNonblocking();
		conn->Init(true);
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	IOProcessor::Add(&tcpread);
}
