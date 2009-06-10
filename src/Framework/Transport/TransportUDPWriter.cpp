#include "System/Buffer.h"
#include "TransportUDPWriter.h"

TransportUDPWriter::TransportUDPWriter()
{
}

TransportUDPWriter::~TransportUDPWriter()
{
	socket.Close();
}

bool TransportUDPWriter::Init(Endpoint &endpoint_)
{
	bool ret;

	endpoint = endpoint_;

	ret = true;
	ret &= socket.Create(UDP);
	ret &= socket.SetNonblocking();
	return ret;
}

void TransportUDPWriter::Write(ByteString &bs)
{
//	Log_Trace("sending %.*s to %s", bs.length, bs.buffer, endpoint.ToString());

	socket.SendTo(bs.buffer, bs.length, endpoint);
}

