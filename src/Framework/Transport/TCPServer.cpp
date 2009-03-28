#include "System/IO/IOProcessor.h"
#include "TCPServer.h"

TCPServer::TCPServer() :
onConnect(this, &TCPServer::OnConnect)
{
}

bool TCPServer::Init(int port)
{
	Log_Trace();

	listener.Create(TCP);
	listener.Listen(port);
	listener.SetNonblocking();
	
	tcpread.fd = listener.fd;
	tcpread.listening = true;
	tcpread.onComplete = &onConnect;
	
	return IOProcessor::Get()->Add(&tcpread);	
}
