#include "TransportTCPReader.h"

#include "System/Common.h"

void TransportTCPConn::OnRead()
{
	int msglength, nread, msgbegin, msgend;
	
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
		Call(reader->onRead);
		
		// move the rest back to the beginning of the buffer
		memcpy(tcpread.data.buffer, tcpread.data.buffer + msgend,
			tcpread.data.length - msgend);
		tcpread.data.length = tcpread.data.length - msgend;
		
		if (tcpread.data.length == 0)
			break;
	}
	while (true);
	
	ioproc->Add(&tcpread);
}
	
void TransportTCPConn::OnClose()
{
	socket.Close();
	delete this;
}



void TransportTCPReader::Init(IOProcessor* ioproc_, int port)
{
	ioproc = ioproc_;
	TCPServer::Init(ioproc_, port);
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

void TransportTCPReader::OnConnect()
{
	TransportTCPConn* conn = new TransportTCPConn(this);
	
	if (listener.Accept(&(conn->GetSocket())))
	{
		Endpoint endpoint;
		conn->GetSocket().GetEndpoint(endpoint);
		
		Log_Message("%s connected", endpoint.ToString());
		
		conn->GetSocket().SetNonblocking();
		conn->Init(ioproc);
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	ioproc->Add(&tcpread);
}
