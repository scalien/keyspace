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

void TransportUDPReader::GetMessage(ByteString& bs_)
{
	bs_ = udpread.data;
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
//	Log_Trace("received %.*s from: %s", udpread.data.length, udpread.data.buffer,
//		udpread.endpoint.ToString());

	Call(userCallback);
	
	IOProcessor::Add(&udpread);
}
