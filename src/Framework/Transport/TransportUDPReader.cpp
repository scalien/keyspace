#include "TransportUDPReader.h"

TransportUDPReader::TransportUDPReader() :
onRead(this, &TransportUDPReader::OnRead)
{
}

void TransportUDPReader::Init(IOProcessor* ioproc_, int port)
{
	ioproc = ioproc_;
	
	socket.Create(UDP);
	socket.Bind(port);
	socket.SetNonblocking();
	
	udpread.fd = socket.fd;
	udpread.data = data;
	udpread.onComplete = &onRead;
	
	ioproc->Add(&udpread);
}

void TransportUDPReader::SetOnRead(Callable* onRead_)
{
	userCallback = onRead_;
}

void TransportUDPReader::GetMessage(ByteString& bs_)
{
	bs_ = udpread.data;
}

void TransportUDPReader::OnRead()
{
	Log_Message("received %.*s from: %s", udpread.data.length, udpread.data.buffer,
		udpread.endpoint.ToString());

	Call(userCallback);
	
	ioproc->Add(&udpread);
}
