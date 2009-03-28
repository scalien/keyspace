#ifndef TRANSPORTUDPREADER_H
#define TRANSPORTUDPREADER_H

#include "System/IO/Socket.h"
#include "Transport.h"
#include "TransportReader.h"

class TransportUDPReader : public TransportReader
{
public:
	TransportUDPReader();
	
	void								Init(int port);

	void								SetOnRead(Callable* onRead);
	void								GetMessage(ByteString& bs_);
	void								Stop();
	void								Continue();
	void								OnRead();
	
private:
	Socket								socket;
	UDPRead								udpread;
	ByteArray<MAX_UDP_MESSAGE_SIZE>		data;
	MFunc<TransportUDPReader>			onRead;
	Callable*							userCallback;
	bool								stopped;
};

#endif
