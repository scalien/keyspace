#include "MemoServer.h"
#include "MemoConn.h"

MemoServer::MemoServer() :
onConnect(this, &MemoServer::OnConnect)
{
}

bool MemoServer::Init(IOProcessor* ioproc_, Scheduler* scheduler_, MemoDB* memodb_, int port)
{
	ioproc = ioproc_;
	scheduler = scheduler_;
	memodb = memodb_;
	
	listener.Create(TCP);
	listener.Listen(port);
	listener.SetNonblocking();
	Log_Message("Listening on socket %d", listener.fd);
	
	tcpread.fd = listener.fd;
	tcpread.listening = true;
	tcpread.onComplete = &onConnect;
	
	return ioproc->Add(&tcpread);
}

void MemoServer::OnConnect()
{
	MemoConn* conn = new MemoConn(memodb);
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