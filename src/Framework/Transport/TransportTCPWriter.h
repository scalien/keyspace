#ifndef TRANSPORT_TCP_WRITER_H
#define TRANSPORT_TCP_WRITER_H

#include "Transport.h"
#include "TransportWriter.h"
#include "TCPConn.h"

class TransportTCPWriter : 
	public TransportWriter,
	public TCPConn<MAX_TCP_MESSAGE_SIZE>
{
public:
	void			Init(Endpoint &endpoint);
	
	void			Connect();
	
	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnWrite();
	virtual void	OnClose();
	
};


#endif
