#ifndef TRANSPORT_UDP_WRITER_H
#define TRANSPORT_UDP_WRITER_H

#include "System/IO/Socket.h"
#include "System/Buffer.h"

class TransportUDPWriter
{
public:
	TransportUDPWriter();
	~TransportUDPWriter();
	
	virtual bool	Init(Endpoint &endpoint);
	virtual	void	Write(ByteString &bs);

private:
	Endpoint		endpoint;
	Socket			socket;
};

#endif
