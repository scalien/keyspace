#ifndef TRANSPORTTCPREADER_H
#define	TRANSPORTTCPREADER_H

#include "Transport.h"
#include "TransportReader.h"
#include "TCPServer.h"
#include "TCPConn.h"

class TransportTCPReader; // forward

class TransportTCPConn : public TCPConn<MAX_TCP_MESSAGE_SIZE>
{
public:
	TransportTCPConn(TransportTCPReader* reader_)
	{
		reader = reader_;
	}
	
	void				OnRead();
	
	void				OnClose();

private:
	TransportTCPReader* reader;
};


class TransportTCPReader : public TransportReader, public TCPServer
{
friend class TransportTCPConn;

public:
	void				Init(IOProcessor* ioproc_, int port);

	void				SetOnRead(Callable* onRead);
	
	void				GetMessage(ByteString& bs_);
	
	void				OnConnect();

private:
	void				SetMessage(ByteString bs_);
	
	IOProcessor*		ioproc;
	Callable*			onRead;
	
	ByteString			bs;
};

#endif
