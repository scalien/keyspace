#include "TransportTCPReader.h"

#include "System/Common.h"

/* class TransportTCPConn */

void TransportTCPConn::OnMessageRead(const ByteString& message)
{
	reader->SetMessage(message);
	
	Call(reader->onRead);
}
	
void TransportTCPConn::OnClose()
{
	Log_Trace();
	
	Close();
	
	reader->OnConnectionClose(this);

	delete this;
}


/* class TransportTCPReader */

TransportTCPReader::~TransportTCPReader()
{
	TransportTCPConn**	it;
	TransportTCPConn*	conn;
	
	for (it = conns.Head(); it != NULL; )
	{
		conn = *it;
		it = conns.Next(it);
		conn->Close();
		delete conn;
	}
	
	Close();
}

bool TransportTCPReader::Init(int port)
{
	onRead = NULL;
	running = true;
	return TCPServer::Init(port);
}

void TransportTCPReader::SetOnRead(Callable* onRead_)
{
	onRead = onRead_;
}

void TransportTCPReader::SetMessage(ByteString msg_)
{
	msg.Set(msg_);
}

void TransportTCPReader::GetMessage(ByteString& msg_)
{
	msg_.Set(msg);
}

bool TransportTCPReader::IsActive()
{
	return tcpread.active;
}

void TransportTCPReader::Stop()
{
	Log_Trace();
	
	TransportTCPConn** it;

	running = false;
	
	for (it = conns.Head(); it != NULL; it = conns.Next(it))
	{
		(*it)->Stop();
		if (running)
			break; // user called Continue(), stop looping
	}
}

void TransportTCPReader::Continue()
{
	Log_Trace();

	TransportTCPConn** it;
	
	running = true;
	
	for (it = conns.Head(); it != NULL; it = conns.Next(it))
	{
		(*it)->Continue();
		if (!running)
			break; // user called Stop(), stop looping
	}
}

void TransportTCPReader::OnConnect()
{
	Log_Trace();
	
	TransportTCPConn* conn = new TransportTCPConn(this);
	
	if (listener.Accept(&(conn->GetSocket())))
	{
		Endpoint endpoint;
		conn->GetSocket().GetEndpoint(endpoint);
		
		Log_Trace("%s connected, fd = %d",
				  endpoint.ToString(), conn->GetSocket().fd);
		
		conn->GetSocket().SetNonblocking();
		conn->Init(true);
		if (!running)
			conn->Stop();
		conns.Append(conn);
	}
	else
	{
		Log_Trace("Accept() failed");
		delete conn;
	}
	
	IOProcessor::Add(&tcpread);
}

void TransportTCPReader::OnConnectionClose(TransportTCPConn* conn)
{
	assert(conns.Remove(conn));
}

