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
	Log_Message("Listening on socket %d", listener.fd);
	
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
		//Log_Message("Accept() succeeded");
		
		conn->socket.SetNonblocking();
		conn->Init(ioproc);
	} else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	ioproc->Add(&tcpread);
}
