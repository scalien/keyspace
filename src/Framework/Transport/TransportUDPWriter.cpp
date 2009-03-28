#include "System/Buffer.h"
#include "TransportUDPWriter.h"

TransportUDPWriter::TransportUDPWriter()
{
	ioproc = NULL;
}

TransportUDPWriter::~TransportUDPWriter()
{
	socket.Close();
}

void TransportUDPWriter::Init(IOProcessor* ioproc_, Scheduler*)
{
	ioproc = ioproc_;
	
	socket.Create(UDP);
	socket.SetNonblocking();
}

void TransportUDPWriter::Start(Endpoint &endpoint_)
{
	endpoint = endpoint_;
}

void TransportUDPWriter::Write(ByteString &bs)
{
	Log_Message("sending %.*s to %s", bs.length, bs.buffer, endpoint.ToString());

	// We use direct sendto here because otherwise
	//  we should buffer the packets here.
	socket.SendTo(bs.buffer, bs.length, endpoint);
}

