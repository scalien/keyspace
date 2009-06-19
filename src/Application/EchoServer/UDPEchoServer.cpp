#include "UDPEchoServer.h"
#include "System/Log.h"
#include <stdio.h>

UDPEchoServer::UDPEchoServer() :
onRead(this, &UDPEchoServer::OnRead),
onWrite(this, &UDPEchoServer::OnWrite)
{
}

bool UDPEchoServer::Init(IOProcessor* ioproc_, Scheduler* scheduler_)
{
	ioproc = ioproc_;
	scheduler = scheduler_;
	
	socket.Create(UDP);
	socket.Bind(UDPECHOSERVER_PORT);
	socket.SetNonblocking();
	Log_Trace("Bound to socket %d", socket.fd);
	
	udpread.fd = socket.fd;
	udpread.data = data;
	udpread.onComplete = &onRead;
	
	udpwrite.fd = socket.fd;
	udpwrite.data = data;
	udpwrite.onComplete = &onWrite;

	return ioproc->Add(&udpread);
}

void UDPEchoServer::OnRead()
{
	Log_Trace();
	
	udpread.data.buffer[udpread.data.length] = '\0';
	
	udpwrite.data.length = udpread.data.length;
	udpwrite.endpoint = udpread.endpoint;
	ioproc->Add(&udpwrite);
}

void UDPEchoServer::OnWrite()
{
	Log_Trace();
	
	ioproc->Add(&udpread);
}
