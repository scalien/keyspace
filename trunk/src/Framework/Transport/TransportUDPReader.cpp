#include "TransportUDPReader.h"
#include "System/IO/IOProcessor.h"
#include "System/Events/EventLoop.h"

TransportUDPReader::TransportUDPReader() :
onRead(this, &TransportUDPReader::OnRead)
{
}

bool TransportUDPReader::Init(int port)
{
	bool ret;
		
	ret = true;
	ret &= socket.Create(UDP);
	ret &= socket.Bind(port);
	ret &= socket.SetNonblocking();
	
	if (!ret)
		return false;
	
	udpread.fd = socket.fd;
	udpread.data = data;
	udpread.onComplete = &onRead;
	
	return IOProcessor::Add(&udpread);
}

void TransportUDPReader::SetOnRead(Callable* onRead_)
{
	userCallback = onRead_;
}

void TransportUDPReader::GetMessage(ByteString& bs)
{
	bs = udpread.data;
}

void TransportUDPReader::GetEndpoint(Endpoint& endpoint)
{
	endpoint = udpread.endpoint;
}

void TransportUDPReader::Stop()
{
	IOProcessor::Remove(&udpread);
}

void TransportUDPReader::Continue()
{
	IOProcessor::Add(&udpread);
}

bool TransportUDPReader::IsActive()
{
	return udpread.active;
}

void TransportUDPReader::OnRead()
{
	Call(userCallback);
	
	IOProcessor::Add(&udpread);
}
