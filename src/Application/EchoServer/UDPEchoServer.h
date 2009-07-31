#ifndef UDPECHOSERVER_H
#define UDPECHOSERVER_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"

#define UDPECHOSERVER_PORT	4000

class UDPEchoServer
{
typedef ByteArray<1024>			Buffer;
typedef MFunc<UDPEchoServer>	Func;

public:
	UDPEchoServer();

	bool			Init(IOProcessor* ioproc_, Scheduler* scheduler_);	
	void			OnRead();
	void			OnWrite();

private:
	IOProcessor*	ioproc;
	Scheduler*		scheduler;
	Socket			socket;
	UDPRead			udpread;
	UDPWrite		udpwrite;
	Buffer			data;
	Func			onRead;
	Func			onWrite;
};

#endif
