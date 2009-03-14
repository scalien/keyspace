#include "TransportTCPReader.h"

#include "Common.h"

void TransportTCPConn::OnRead()
{
	int msglength, nread, msgbegin, msgend;
	
	msglength = strntol(tcpread.data.buffer, tcpread.data.length, &nread);
	if (nread > 0 && tcpread.data.length > nread)
	{
		if (tcpread.data.buffer[nread] == ':')
		{
			msgbegin = nread + 1;
			msgend = nread + 1 + msglength;

			if (tcpread.data.length >= nread + 1 + msglength)
			{
				
				reader->SetMessage(
					ByteString(msglength, msglength, tcpread.data.buffer + msgbegin)
					);
				Call(onRead);
				
				// move the rest back to the beginning of the buffer
				memcpy(tcpread.data.buffer, tcpread.data.buffer + msgend,
					tcpread.data.length - msgend);
				tcpread.data.length = tcpread.data.length - msgend;
			}
			else
			{
				// request just the right amount
				tcpread.requested = msgend - tcpread.data.length;
				ioproc->Add(&tcpread);
				return;
			}
		}
		else
			ASSERT_FAIL();
	}
	
	tcpread.requested = IO_READ_ANY;
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
	TransportTCPConn* conn = new TransportTCPConn(this, onRead);
	
	if (listener.Accept(&(conn->GetSocket())))
	{
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
