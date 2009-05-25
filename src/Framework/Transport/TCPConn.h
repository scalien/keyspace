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

template<int bufferSize = MAX_TCP_MESSAGE_SIZE>
class TCPConn
{
public:
	TCPConn			*next;

	TCPConn();
	virtual ~TCPConn();
	
	void			Init(bool startRead = true);
	void			Connect(Endpoint &endpoint_, unsigned timeout);
	void			Close();

	Socket&			GetSocket() { return socket; }
	
protected:
	typedef DynArray<bufferSize>			Buffer;
	typedef Queue<Buffer, &Buffer::next>	BufferQueue;

	enum State 
	{
		DISCONNECTED,
		CONNECTED,
		CONNECTING
	};
	
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
	
	void			AsyncRead(bool start = true);
	void			Append(const char* data, int count);
	void			Write(const char* data, int count, bool flush = true);
	void			WritePending();
};


template<int bufferSize>
TCPConn<bufferSize>::TCPConn() :
connectTimeout(&onConnectTimeout),
onRead(this, &TCPConn::OnRead),
onWrite(this, &TCPConn::OnWrite),
onClose(this, &TCPConn::OnClose),
onConnect(this, &TCPConn::OnConnect),
onConnectTimeout(this, &TCPConn::OnConnectTimeout)
{
	state = DISCONNECTED;
}


template<int bufferSize>
TCPConn<bufferSize>::~TCPConn()
{
	Buffer* buf;

	while ((buf = writeQueue.Get()) != NULL)
		delete buf;
	
	Close();
}


template<int bufferSize>
void TCPConn<bufferSize>::Init(bool startRead)
{
	Log_Trace();
	
	state = CONNECTED;
	
	readBuffer.Clear();
	AsyncRead(startRead);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onWrite;
	tcpwrite.onClose = &onClose;
}

template<int bufferSize>
void TCPConn<bufferSize>::OnWrite()
{
	Log_Trace();

	Log_Message("Written %d bytes", tcpwrite.data.length);

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

template<int bufferSize>
void TCPConn<bufferSize>::OnConnect()
{
	Log_Trace();
	
	state = CONNECTED;
	
	EventLoop::Remove(&connectTimeout);
	WritePending();
}

template<int bufferSize>
void TCPConn<bufferSize>::OnConnectTimeout()
{
	Log_Trace();
}

template<int bufferSize>
void TCPConn<bufferSize>::AsyncRead(bool start)
{
	Log_Trace();
	
	tcpread.fd = socket.fd;
	tcpread.data = readBuffer;
	tcpread.onComplete = &onRead;
	tcpread.onClose = &onClose;
	tcpread.requested = IO_READ_ANY;
	if (start)
		IOProcessor::Add(&tcpread);
}

template<int bufferSize>
void TCPConn<bufferSize>::Write(const char *data, int count, bool flush)
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

template<int bufferSize>
void TCPConn<bufferSize>::WritePending()
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

template<int bufferSize>
void TCPConn<bufferSize>::Close()
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

template<int bufferSize>
void TCPConn<bufferSize>::Connect(Endpoint &endpoint_, unsigned timeout)
{
	Log_Message("endpoint_ = %s", endpoint_.ToString());

	bool ret;

	if (state != DISCONNECTED)
		return;
		
	Init(false);
	state = CONNECTING;

	socket.Create(TCP);
	socket.SetNonblocking();
	ret = socket.Connect(endpoint_);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onConnect;
	// zero indicates for IOProcessor that we are waiting for connect event
	tcpwrite.data.length = 0;
	
	IOProcessor::Add(&tcpwrite);

	if (timeout > 0)
	{
		Log_Message("starting timeout with %d", timeout);
		
		connectTimeout.SetDelay(timeout);
		EventLoop::Reset(&connectTimeout);
	}
}

#endif
