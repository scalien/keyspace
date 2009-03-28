#include "TransportTCPReader.h"

#include "System/Common.h"

void TransportTCPConn::OnRead()
{
	int msglength, nread, msgbegin, msgend;
	Endpoint endpoint;
	
	GetSocket().GetEndpoint(endpoint);
	Log_Message("endpoint = %s", endpoint.ToString());
	
	tcpread.requested = IO_READ_ANY;
	
	do
	{
		msglength = strntol(tcpread.data.buffer, tcpread.data.length, &nread);
		
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

		
		reader->SetMessage(
			ByteString(msglength, msglength, tcpread.data.buffer + msgbegin)
			);
		
		if (!reader->stopped)
			Call(reader->onRead);
		
		// move the rest back to the beginning of the buffer
		memcpy(tcpread.data.buffer, tcpread.data.buffer + msgend,
			tcpread.data.length - msgend);
		tcpread.data.length = tcpread.data.length - msgend;
		
		if (tcpread.data.length == 0)
			break;
	}
	while (true);
	
	IOProcessor::Get()->Add(&tcpread);
}
	
void TransportTCPConn::OnClose()
{
	socket.Close();
	delete this;
}



void TransportTCPReader::Init(int port)
{
	TCPServer::Init(port);
	Log_Message("fd = %d", listener.fd);
	stopped = false;
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
	stopped = true;
}

void TransportTCPReader::Continue()
{
	stopped = false;
}

void TransportTCPReader::OnConnect()
{
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
	
	IOProcessor::Get()->Add(&tcpread);
}
