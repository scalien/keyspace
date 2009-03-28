#ifndef TRANSPORT_TCP_WRITER_H
#define TRANSPORT_TCP_WRITER_H

#include "System/Events/Scheduler.h"
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
	TransportTCPWriter();
	~TransportTCPWriter();
	
	// TransportWriter interface
	virtual void	Init(IOProcessor* ioproc_, Scheduler* scheduler_);
	virtual void	Start(Endpoint &endpoint_);
	virtual void	Write(ByteString &bs);

private:
	void			Connect();
	void			OnConnect();
	void			OnConnectTimeout();
	
	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnClose();

	enum State 
	{
		DISCONNECTED,
		CONNECTED,
		CONNECTING
	};
	
	State						state;
	Endpoint					endpoint;
	Scheduler*					scheduler;	
	MFunc<TransportTCPWriter>	onConnect;
	MFunc<TransportTCPWriter>	onConnectTimeout;
	CdownTimer					connectTimeout;
};


#endif
