#ifndef TRANSPORT_UDP_WRITER_H
#define TRANSPORT_UDP_WRITER_H

#include "System/IO/Socket.h"
#include "TransportWriter.h"

class TransportUDPWriter : public TransportWriter
{
public:
	TransportUDPWriter();
	~TransportUDPWriter();
	
	// TransportWriter interface
	virtual bool	Init(Endpoint &endpoint);
	virtual	void	Write(ByteString &bs);

private:
	Endpoint		endpoint;
	Socket			socket;
};

#endif
