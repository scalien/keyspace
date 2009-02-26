#include "System/IO/IOProcessor.h"
#include "TCPServer.h"

TCPServer::TCPServer() :
onConnect(this, &TCPServer::OnConnect)
{
}

bool TCPServer::Init(IOProcessor* ioproc_, int port)
{
	ioproc = ioproc_;
	
	listener.Create(TCP);
	listener.Listen(port);
	listener.SetNonblocking();
	
	tcpread.fd = listener.fd;
	tcpread.listening = true;
	tcpread.onComplete = &onConnect;
	
	return ioproc->Add(&tcpread);	
}
