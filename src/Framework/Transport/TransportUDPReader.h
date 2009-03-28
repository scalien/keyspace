#ifndef TRANSPORTUDPREADER_H
#define TRANSPORTUDPREADER_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"
#include "Transport.h"
#include "TransportReader.h"

class TransportUDPReader : public TransportReader
{
public:
	TransportUDPReader();
	
	void								Init(IOProcessor* ioproc_, int port);

	void								SetOnRead(Callable* onRead);
	void								GetMessage(ByteString& bs_);
	void								Stop();
	void								Continue();
	void								OnRead();
	
private:
	IOProcessor*						ioproc;
	Socket								socket;
	UDPRead								udpread;
	ByteArray<MAX_UDP_MESSAGE_SIZE>		data;
	MFunc<TransportUDPReader>			onRead;
	Callable*							userCallback;
	bool								stopped;
};

#endif
