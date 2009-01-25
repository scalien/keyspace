#ifndef MEMOCONN_H
#define MEMOCONN_H

#include "MemoDB.h"
#include "MemoClientMsg.h"
#include "IOProcessor.h"
#include "Scheduler.h"
#include "Socket.h"

class MemoConn
{
public:
	MemoConn(MemoDB* memoserver);

	IOProcessor*	ioproc;
	Socket			socket;
	TCPRead			tcpread;
	TCPWrite		tcpwrite;
	ByteArray<1024>	data;
	ByteString		bs;
	
	MemoDB*			memodb;
	MemoMsg			msg;
	
	bool			Init(IOProcessor* ioproc_);
	void			Shutdown();
	
	void			OnWelcome();
	void			OnRead();
	void			OnWrite();
	void			OnCloseRead();
	void			OnCloseWrite();
	void			OnMemoDBComplete();
	
	MFunc<MemoConn>	onWelcome;
	MFunc<MemoConn>	onRead;
	MFunc<MemoConn>	onWrite;
	MFunc<MemoConn>	onCloseRead;
	MFunc<MemoConn>	onCloseWrite;
	MFunc<MemoConn>	onMemoDBComplete;
};

#endif