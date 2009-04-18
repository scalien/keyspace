#ifndef TRANSPORT_TCP_WRITER_H
#define TRANSPORT_TCP_WRITER_H

#include "System/Events/Timer.h"
#include "System/IO/IOProcessor.h"
#include "Transport.h"
#include "TransportWriter.h"
#include "TCPConn.h"

class TransportTCPWriter : 
	public TransportWriter,
	public TCPConn<MAX_TCP_MESSAGE_SIZE>
{
public:
	
	// TransportWriter interface
	virtual bool	Init(Endpoint &endpoint_);
	virtual void	Write(ByteString &bs);

private:
	void			Connect();
	void			OnConnect();
	void			OnConnectTimeout();
	
	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnClose();

	Endpoint		endpoint;
};


#endif
