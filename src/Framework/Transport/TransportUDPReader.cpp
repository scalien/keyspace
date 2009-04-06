#include "TransportUDPReader.h"
#include "System/IO/IOProcessor.h"
#include "System/Events/EventLoop.h"

TransportUDPReader::TransportUDPReader() :
onRead(this, &TransportUDPReader::OnRead)
{
}

void TransportUDPReader::Init(int port)
{
	
	stopped = false;
	
	socket.Create(UDP);
	socket.Bind(port);
	socket.SetNonblocking();
	
	udpread.fd = socket.fd;
	udpread.data = data;
	udpread.onComplete = &onRead;
	
	IOProcessor::Add(&udpread);
}

void TransportUDPReader::SetOnRead(Callable* onRead_)
{
	userCallback = onRead_;
}

void TransportUDPReader::GetMessage(ByteString& bs_)
{
	bs_ = udpread.data;
}

void TransportUDPReader::Stop()
{
	stopped = false;
}

void TransportUDPReader::Continue()
{
	stopped = true;
}

void TransportUDPReader::OnRead()
{
	Log_Message("received %.*s from: %s", udpread.data.length, udpread.data.buffer,
		udpread.endpoint.ToString());

	if (!stopped)
		Call(userCallback);
	
	IOProcessor::Add(&udpread);
}
