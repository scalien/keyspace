#ifndef TRANSPORTREADER_H
#define	TRANSPORTREADER_H

#include "TCPServer.h"
#include "TCPConn.h"

class TransportReader
{
public:
	virtual void	Init(IOProcessor* ioproc_, int port_) = 0;

	virtual void	SetOnRead(Callable* onRead) = 0;
	
	virtual void	GetMessage(ByteString& bs) = 0;
};


class TransportTCPConn : public TCPConn<MAX_TCP_MESSAGE_SIZE>
{
};

class TransportTCPReader : public TransportReader, public TCPServer
{
public:
	void			Init(IOProcessor* ioproc_, int port_);

	void			SetOnRead(Callable* onRead);
	
	void			GetMessage(ByteString& bs);
	
	void			OnConnect();
};

#endif
