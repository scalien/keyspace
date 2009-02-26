#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "System/Events/Callable.h"
#include "System/IO/Socket.h"
#include "System/IO/IOOperation.h"

class TCPServer
{
public:
	TCPServer();
	
	bool				Init(IOProcessor* ioproc_, int port);
	
protected:
	TCPRead				tcpread;
	Socket				listener;
	IOProcessor*		ioproc;
	MFunc<TCPServer>	onConnect;
	
	virtual void		OnConnect() = 0;
};

#endif
