#ifndef TCPCONN_H
#define TCPCONN_H

#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Callable.h"
#include "System/Events/Timer.h"
#include "System/Events/EventLoop.h"
#include "System/Containers/List.h"
#include "System/Buffer.h"
#include "Transport.h"

#define TCP_CONNECT_TIMEOUT 3000

template<int bufferSize = MAX_TCP_MESSAGE_SIZE>
class TCPConn
{
public:
	TCPConn();
	virtual ~TCPConn();
	
	void			Init(bool startRead = true);
	void			Connect(Endpoint &endpoint_, int timeout);

	Socket&			GetSocket() { return socket; }
	
protected:
	typedef DynArray<bufferSize> Buffer;

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
	List<Buffer*>	writeQueue;
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
	void			Write(const char* data, int count, bool flush = true);
	void			WritePending();
	void			Close();
};


template<int bufferSize>
TCPConn<bufferSize>::TCPConn() :
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
	Buffer** it;

	for (it = writeQueue.Head(); it != NULL; it = writeQueue.Next(it))
		delete *it;	
	
	Close();
}


template<int bufferSize>
void TCPConn<bufferSize>::Init(bool startRead)
{
	state = CONNECTED;
	
	AsyncRead(startRead);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onWrite;
	tcpwrite.onClose = &onClose;
	
	// preallocate a buffer for writeQueue
	Buffer* buf = new Buffer;
	writeQueue.Append(buf);
}

template<int bufferSize>
void TCPConn<bufferSize>::OnWrite()
{
	Log_Trace();
	
	Buffer** it;
	Buffer* buf;
	Buffer* last;
	
	it = writeQueue.Head();
	buf = *it;
	buf->Clear();
	tcpwrite.data.Clear();
	
	if (writeQueue.Size() > 1)
	{
		writeQueue.Remove(it);
		it = writeQueue.Tail();
		last = *it;
		if (last->length)
			writeQueue.Append(buf);
		else
			delete buf;
		
		WritePending();
	}	
}

template<int bufferSize>
void TCPConn<bufferSize>::OnConnect()
{
	Log_Trace();
	
	EventLoop::Get()->Remove(&connectTimeout);
}

template<int bufferSize>
void TCPConn<bufferSize>::OnConnectTimeout()
{
	Log_Trace();
}

template<int bufferSize>
void TCPConn<bufferSize>::AsyncRead(bool start)
{
	tcpread.fd = socket.fd;
	tcpread.data = readBuffer;
	tcpread.onComplete = &onRead;
	tcpread.onClose = &onClose;
	tcpread.requested = IO_READ_ANY;
	if (start)
		IOProcessor::Get()->Add(&tcpread);
}


template<int bufferSize>
void TCPConn<bufferSize>::Write(const char *data, int count, bool flush)
{
	Log_Trace();
	
	Buffer* buf;
	Buffer* head;
	
	head = *writeQueue.Head();
	buf = *writeQueue.Tail();
	if ((tcpwrite.active && head == buf) || buf->size - buf->length < count)
	{
		// TODO max queue size
		buf = new Buffer;
		writeQueue.Append(buf);
	}
	
	memcpy(buf->buffer + buf->length, data, count);
	buf->length += count;
	
	if (!tcpwrite.active && flush)
		WritePending();
}

template<int bufferSize>
void TCPConn<bufferSize>::WritePending()
{
	Log_Trace();
	
	Buffer* buf;
	
	buf = *writeQueue.Head();
	
	if (buf->length)
	{
		tcpwrite.data = *buf;
		tcpwrite.transferred = 0;
		
		IOProcessor::Get()->Add(&tcpwrite);		
	}	
}

template<int bufferSize>
void TCPConn<bufferSize>::Close()
{
	if (tcpread.active)
		IOProcessor::Get()->Remove(&tcpread);
	
	if (tcpwrite.active)
		IOProcessor::Get()->Remove(&tcpwrite);
	
	socket.Close();
	state = DISCONNECTED;
}

template<int bufferSize>
void TCPConn<bufferSize>::Connect(Endpoint &endpoint_, int timeout)
{
	Log_Message("endpoint_ = %s", endpoint_.ToString());

	if (state != DISCONNECTED)
		return;
	
	bool ret;
	
	Init( false);
	state = CONNECTING;

	socket.Create(TCP);
	socket.SetNonblocking();
	ret = socket.Connect(endpoint_);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onConnect;
	tcpwrite.data.length = 0;
	
	IOProcessor::Get()->Add(&tcpwrite);

	if (timeout)
	{
		connectTimeout.SetDelay(timeout);
		EventLoop::Get()->Reset(&connectTimeout);
	}
}

#endif
