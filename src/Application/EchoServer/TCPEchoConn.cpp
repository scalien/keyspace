#include "TCPEchoConn.h"

TCPEchoConn::TCPEchoConn() :
onWelcome(this, &TCPEchoConn::OnWelcome),
onRead(this, &TCPEchoConn::OnRead),
onWrite(this, &TCPEchoConn::OnWrite),
onCloseRead(this, &TCPEchoConn::OnCloseRead),
onCloseWrite(this, &TCPEchoConn::OnCloseWrite)
{
}

bool TCPEchoConn::Init(IOProcessor* ioproc_)
{
	ioproc = ioproc_;
	
	tcpwrite.fd = socket.fd;
	tcpwrite.data = data;
	
	#define WELCOME_MSG "Welcome to TCPEchoServer!\n\n"
	sprintf(tcpwrite.data.buffer, WELCOME_MSG);
	tcpwrite.data.length = strlen(WELCOME_MSG);
	tcpwrite.onComplete = &onWelcome;
	tcpwrite.onClose = &onCloseWrite;
	ioproc->Add(&tcpwrite);

	return true;
}

void TCPEchoConn::OnWelcome()
{
	Log_Trace();
	
	tcpread.fd = socket.fd;
	tcpread.data = data;
	tcpread.requested = IO_READ_ANY;
	tcpread.onComplete = &onRead;
	tcpread.onClose = &onCloseRead;
	ioproc->Add(&tcpread);
	
	tcpwrite.onComplete = &onWrite;
}

void TCPEchoConn::Shutdown()
{
	Log_Trace();

	socket.Close();
}

void TCPEchoConn::OnRead()
{
	Log_Trace();
	
	tcpwrite.data.length = tcpread.data.length;
	tcpwrite.transferred = 0;
	ioproc->Add(&tcpwrite);
}

void TCPEchoConn::OnWrite()
{
	Log_Trace();
	
	tcpread.data.length = 0;
	ioproc->Add(&tcpread);
}

void TCPEchoConn::OnCloseRead()
{
	Log_Trace();
	
	Shutdown();
	delete this;
}

void TCPEchoConn::OnCloseWrite()
{
	Log_Trace();
	
	Shutdown();
	delete this;
}
