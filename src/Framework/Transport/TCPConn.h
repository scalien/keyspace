#ifndef TCPCONN_H
#define TCPCONN_H

#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/Socket.h"
#include "System/Events/Callable.h"
#include "System/Containers/List.h"
#include "System/Buffer.h"

template<int bufferSize = 4096>
class TCPConn
{
public:
	TCPConn();
	virtual ~TCPConn();
	
	void			Init(IOProcessor* ioproc_, bool startRead = true);
	Socket&			GetSocket() { return socket; }
	
protected:
	typedef ByteArray<bufferSize> Buffer;
	
	Socket			socket;
	TCPRead			tcpread;
	TCPWrite		tcpwrite;
	IOProcessor*	ioproc;
	Buffer			readBuffer;
	List<Buffer*>	writeQueue;
	
	MFunc<TCPConn>	onRead;
	MFunc<TCPConn>	onWrite;
	MFunc<TCPConn>	onClose;
	
	virtual void	OnRead() = 0;
	virtual void	OnWrite();
	virtual void	OnClose() = 0;
	
	void			Write(const char* data, int count, bool flush = true);
	void			WritePending();
	void			Close();
};


template<int bufferSize>
TCPConn<bufferSize>::TCPConn() :
onRead(this, &TCPConn::OnRead),
onWrite(this, &TCPConn::OnWrite),
onClose(this, &TCPConn::OnClose)
{
	ioproc = NULL;
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
void TCPConn<bufferSize>::Init(IOProcessor* ioproc_, bool startRead)
{
	ioproc = ioproc_;
	
	tcpread.fd = socket.fd;
	tcpread.data = readBuffer;
	tcpread.onComplete = &onRead;
	tcpread.onClose = &onClose;
	tcpread.requested = IO_READ_ANY;
	if (startRead)
		ioproc->Add(&tcpread);
	
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
void TCPConn<bufferSize>::Write(const char *data, int count, bool activate)
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
	
	if (!tcpwrite.active && activate)
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
		
		ioproc->Add(&tcpwrite);		
	}	
}

template<int bufferSize>
void TCPConn<bufferSize>::Close()
{
	if (tcpread.active)
		ioproc->Remove(&tcpread);
	
	if (tcpwrite.active)
		ioproc->Remove(&tcpwrite);
	
	socket.Close();
}

#endif
