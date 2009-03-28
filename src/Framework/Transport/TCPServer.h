#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "System/Events/Callable.h"
#include "System/IO/Socket.h"
#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"

class TCPServer
{
public:
	TCPServer();
	
	bool				Init(int port);
	
protected:
	TCPRead				tcpread;
	Socket				listener;
	MFunc<TCPServer>	onConnect;
	
	virtual void		OnConnect() = 0;
};

#endif
