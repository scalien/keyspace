#ifndef UDPECHOSERVER_H
#define UDPECHOSERVER_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"

#define UDPECHOSERVER_PORT	4000

class UDPEchoServer
{
public:
	UDPEchoServer();

	IOProcessor*			ioproc;
	Scheduler*				scheduler;
	Socket					socket;
	UDPRead					udpread;
	UDPWrite				udpwrite;
	ByteArray<1024>			data;

	bool					Init(IOProcessor* ioproc_, Scheduler* scheduler_);
	
	void					OnRead();
	void					OnWrite();
	
	MFunc<UDPEchoServer>	onRead;
	MFunc<UDPEchoServer>	onWrite;
};

#endif
