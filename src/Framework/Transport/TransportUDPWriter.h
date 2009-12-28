#ifndef TRANSPORT_UDP_WRITER_H
#define TRANSPORT_UDP_WRITER_H

#include "System/IO/Endpoint.h"
#include "System/IO/Socket.h"
#include "System/Buffer.h"

class TransportUDPWriter
{
public:
	TransportUDPWriter();
	virtual ~TransportUDPWriter();
	
	virtual bool	Init(Endpoint &endpoint);
	virtual	void	Write(ByteString &bs);

private:
	Endpoint		endpoint;
	Socket			socket;
};

#endif
