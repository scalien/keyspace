#ifndef TCPECHOCONN_H
#define TCPECHOCONN_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"

class TCPEchoConn
{
public:
	TCPEchoConn();

	IOProcessor*	ioproc;
	Socket			socket;
	TCPRead			tcpread;
	TCPWrite		tcpwrite;
	ByteArray<1024>	data;
	
	bool			Init(IOProcessor* ioproc_);
	void			Shutdown();
	
	void			OnWelcome();
	void			OnRead();
	void			OnWrite();
	void			OnCloseRead();
	void			OnCloseWrite();	
	
	MFunc<TCPEchoConn>		onWelcome;
	MFunc<TCPEchoConn>		onRead;
	MFunc<TCPEchoConn>		onWrite;
	MFunc<TCPEchoConn>		onCloseRead;
	MFunc<TCPEchoConn>		onCloseWrite;
};

#endif
