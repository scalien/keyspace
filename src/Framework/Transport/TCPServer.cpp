#include "System/IO/IOProcessor.h"
#include "TCPServer.h"

TCPServer::TCPServer() :
onConnect(this, &TCPServer::OnConnect)
{
}

bool TCPServer::Init(int port)
{
	Log_Trace();

	bool ret;

	ret = true;
	ret &= listener.Create(Socket::TCP);
	ret &= listener.Listen(port);
	ret &= listener.SetNonblocking();
	if (!ret)
		return false;
	
	tcpread.fd = listener.fd;
	tcpread.listening = true;
	tcpread.onComplete = &onConnect;
	
	return IOProcessor::Add(&tcpread);	
}
