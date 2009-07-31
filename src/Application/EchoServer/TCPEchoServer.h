#ifndef TCPECHOSERVER_H
#define TCPECHOSERVER_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"

#define TCPECHOSERVER_PORT	3000

class TCPEchoServer
{
typedef MFunc<TCPEchoServer> Func;

public:
	TCPEchoServer();
	
private:
	bool			Init(IOProcessor* ioproc_, Scheduler* scheduler_);
	
	void			OnConnect();
	IOProcessor*	ioproc;
	Scheduler*		scheduler;
	Socket			listener;
	TCPRead			tcpread;
	Func			onConnect;
};

#endif
