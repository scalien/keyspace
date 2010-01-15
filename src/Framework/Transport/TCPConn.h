#ifndef TCPCONN_H
#define TCPCONN_H

//===================================================================
//
// TCPConn.h:
//
//	Async tcp connection with automatic write queue management.
//
//===================================================================

#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Callable.h"
#include "System/Events/Timer.h"
#include "System/Events/EventLoop.h"
#include "System/Containers/List.h"
#include "System/Containers/Queue.h"
#include "System/Buffer.h"
#include "Transport.h"

#define TCP_CONNECT_TIMEOUT 3000

template<int bufSize = MAX_TCP_MESSAGE_SIZE>
class TCPConn
{
public:
	enum State 
	{
		DISCONNECTED,
		CONNECTED,
		CONNECTING
	};

	TCPConn			*next;

	TCPConn();
	virtual ~TCPConn();
	
	void			Init(bool startRead = true);
	void			Connect(Endpoint &endpoint_, unsigned timeout);
	void			Close();

	Socket&			GetSocket() { return socket; }
	const State		GetState() { return state; }

	void			AsyncRead(bool start = true);
	void			Write(const char* data, int count, bool flush = true);
	
protected:
	typedef DynArray<bufSize> Buffer;
	typedef Queue<Buffer, &Buffer::next> BufferQueue;

	
	State			state;
	Socket			socket;
	TCPRead			tcpread;
	TCPWrite		tcpwrite;
	Buffer			readBuffer;
	BufferQueue		writeQueue;
	CdownTimer		connectTimeout;
	
	MFunc<TCPConn>	onRead;
	MFunc<TCPConn>	onWrite;
	MFunc<TCPConn>	onClose;
	MFunc<TCPConn>	onConnect;
	MFunc<TCPConn>	onConnectTimeout;
	
	virtual void	OnRead() = 0;
	virtual void	OnWrite();
	virtual void	OnClose() = 0;
	virtual void	OnConnect();
	virtual void	OnConnectTimeout();
	
	void			Append(const char* data, int count);
	void			WritePending();
};


template<int bufSize>
TCPConn<bufSize>::TCPConn() :
connectTimeout(&onConnectTimeout),
onRead(this, &TCPConn::OnRead),
onWrite(this, &TCPConn::OnWrite),
onClose(this, &TCPConn::OnClose),
onConnect(this, &TCPConn::OnConnect),
onConnectTimeout(this, &TCPConn::OnConnectTimeout)
{
	state = DISCONNECTED;
}


template<int bufSize>
TCPConn<bufSize>::~TCPConn()
{
	Buffer* buf;

	while ((buf = writeQueue.Get()) != NULL)
		delete buf;
	
	Close();
}


template<int bufSize>
void TCPConn<bufSize>::Init(bool startRead)
{
	Log_Trace();
	
	state = CONNECTED;
	
	readBuffer.Clear();

	assert(tcpread.active == false);
	assert(tcpwrite.active == false);

	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onWrite;
	tcpwrite.onClose = &onClose;

	AsyncRead(startRead);
}

template<int bufSize>
void TCPConn<bufSize>::Connect(Endpoint &endpoint_, unsigned timeout)
{
	Log_Trace("endpoint_ = %s", endpoint_.ToString());

	bool ret;

	if (state != DISCONNECTED)
		return;
		
	Init(false);
	state = CONNECTING;

	socket.Create(Socket::TCP);
	socket.SetNonblocking();
	ret = socket.Connect(endpoint_);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onConnect;
	// zero indicates for IOProcessor that we are waiting for connect event
	tcpwrite.data.length = 0;
	
	IOProcessor::Add(&tcpwrite);

	if (timeout > 0)
	{
		Log_Trace("starting timeout with %d", timeout);
		
		connectTimeout.SetDelay(timeout);
		EventLoop::Reset(&connectTimeout);
	}
}

template<int bufSize>
void TCPConn<bufSize>::OnWrite()
{
	Log_Trace("Written %d bytes", tcpwrite.data.length);
	Log_Trace("Written: %.*s", tcpwrite.data.length, tcpwrite.data.buffer);

	Buffer* buf;
	
	buf = writeQueue.Get();
	buf->Clear();
	tcpwrite.data.Clear();
	
	if (writeQueue.Size() == 0)
		writeQueue.Append(buf);
	else
	{
		delete buf;
		WritePending();
	}
}

template<int bufSize>
void TCPConn<bufSize>::OnConnect()
{
	Log_Trace();
	
	state = CONNECTED;
	tcpwrite.onComplete = &onWrite;
	
	EventLoop::Remove(&connectTimeout);
	WritePending();
}

template<int bufSize>
void TCPConn<bufSize>::OnConnectTimeout()
{
	Log_Trace();
}

template<int bufSize>
void TCPConn<bufSize>::AsyncRead(bool start)
{
	Log_Trace();
	
	tcpread.fd = socket.fd;
	tcpread.data = readBuffer;
	tcpread.onComplete = &onRead;
	tcpread.onClose = &onClose;
	tcpread.requested = IO_READ_ANY;
	if (start)
		IOProcessor::Add(&tcpread);
	else
		Log_Trace("not posting read");
}

template<int bufSize>
void TCPConn<bufSize>::Write(const char *data, int count, bool flush)
{
	//Log_Trace();
	
	if (state == DISCONNECTED)
		return;
	
	Buffer* buf;

	if (data && count > 0)
	{
		buf = writeQueue.Tail();

		if (!buf ||
			(tcpwrite.active && writeQueue.Size() == 1) || 
			(buf->length > 0 && buf->Remaining() < count))
		{
			buf = new Buffer;
			writeQueue.Append(buf);
		}

		buf->Append(data, count);
	}

	if (flush)
		WritePending();
}

template<int bufSize>
void TCPConn<bufSize>::WritePending()
{
//	Log_Trace();
	
	if (state == DISCONNECTED)
		return;

	
	Buffer* buf;
	
	if (tcpwrite.active)
		return;
	
	buf = writeQueue.Head();
	
	if (buf && buf->length > 0)
	{
		tcpwrite.data = *buf;
		tcpwrite.transferred = 0;
		
		IOProcessor::Add(&tcpwrite);
	}	
}

template<int bufSize>
void TCPConn<bufSize>::Close()
{
	Log_Trace();

	EventLoop::Remove(&connectTimeout);

	if (tcpread.active)
		IOProcessor::Remove(&tcpread);

	if (tcpwrite.active)
		IOProcessor::Remove(&tcpwrite);

	socket.Close();
	state = DISCONNECTED;
	
	// Discard unnecessary buffers if there are any.
	// Keep the last one, so that when the connection
	// is reused it isn't reallocated.
	while (writeQueue.Size() > 0)
	{
		Buffer* buf = writeQueue.Get();
		if (writeQueue.Size() == 0)
		{
			buf->Clear();
			writeQueue.Append(buf);
			break;
		}
		else
			delete buf;
	}
	
	// TODO ? if the last buffer size exceeds some limit, 
	// reallocate it to a normal value
}

#endif
