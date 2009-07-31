#ifndef TCPECHOCONN_H
#define TCPECHOCONN_H

#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Scheduler.h"

class TCPEchoConn
{
typedef ByteArray<1024>		Buffer;
typedef MFunc<TCPEchoConn>	Func;

public:
	TCPEchoConn();

	bool			Init(IOProcessor* ioproc_);
	void			Shutdown();	
	void			OnWelcome();
	void			OnRead();
	void			OnWrite();
	void			OnCloseRead();
	void			OnCloseWrite();	


private:
	IOProcessor*	ioproc;
	Socket			socket;
	TCPRead			tcpread;
	TCPWrite		tcpwrite;
	Buffer			data;
	Func			onWelcome;
	Func			onRead;
	Func			onWrite;
	Func			onCloseRead;
	Func			onCloseWrite;
};

#endif
