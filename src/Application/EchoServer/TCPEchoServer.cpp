#include "TCPEchoServer.h"
#include <stdio.h>
#include "System/Log.h"
#include "TCPEchoConn.h"

TCPEchoServer::TCPEchoServer() :
onConnect(this, &TCPEchoServer::OnConnect)
{
}

bool TCPEchoServer::Init(IOProcessor* ioproc_, Scheduler* scheduler_)
{
	ioproc = ioproc_;
	scheduler = scheduler_;
	
	listener.Create(TCP);
	listener.Listen(TCPECHOSERVER_PORT);
	listener.SetNonblocking();

	tcpread.fd = listener.fd;
	tcpread.listening = true;
	tcpread.onComplete = &onConnect;
	
	return ioproc->Add(&tcpread);
}

void TCPEchoServer::OnConnect()
{
	TCPEchoConn* conn = new TCPEchoConn();
	if (listener.Accept(&(conn->socket)))
	{
		conn->socket.SetNonblocking();
		conn->Init(ioproc);
	} else
	{
		delete conn;
	}
	
	ioproc->Add(&tcpread);
}
