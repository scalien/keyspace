#ifndef MESSAGECONN_H
#define MESSAGECONN_H

#include "TCPConn.h"

template<int bufSize = MAX_TCP_MESSAGE_SIZE>
class MessageConn : public TCPConn<bufSize>
{
public:
	MessageConn()
	{
		running = true;
	}
	
	virtual void	OnMessageRead(const ByteString& msg) = 0;
	virtual void	OnClose() = 0;
	virtual void	OnRead();
	
	virtual void	Stop();
	virtual void	Continue();

protected:
	bool			running;
};

template<int bufSize>
void MessageConn<bufSize>::OnRead()
{
	TCPRead& tcpread = TCPConn<bufSize>::tcpread;
	unsigned pos, msglength, nread, msgbegin, msgend;
	ByteString msg;
	
	Log_Trace("Read buffer: %.*s", tcpread.data.length, tcpread.data.buffer);
	
	tcpread.requested = IO_READ_ANY;

	pos = 0;

	while(running)
	{
		msglength = strntouint64(tcpread.data.buffer + pos,
								 tcpread.data.length - pos,
								 &nread);
		
		if (msglength > tcpread.data.size - NumLen(msglength) - 1)
		{
			OnClose();
			return;
		}
		
		if (nread == 0 || (unsigned) tcpread.data.length - pos <= nread)
			break;
			
		if (tcpread.data.buffer[pos + nread] != ':')
		{
			Log_Trace("Message protocol error");
			OnClose();
			return;
		}
	
		msgbegin = pos + nread + 1;
		msgend = pos + nread + 1 + msglength;
		
		if ((unsigned) tcpread.data.length < msgend)
		{
			tcpread.requested = msgend - pos;
			break;
		}

		msg = ByteString(msglength, msglength, tcpread.data.buffer + msgbegin);
		OnMessageRead(msg);
		
		pos = msgend;
		
		// if the user called Close() in OnMessageRead()
		if (TCPConn<bufSize>::state != TCPConn<bufSize>::CONNECTED)
			return;
		
		if (tcpread.data.length == msgend)
			break;
	}
	
	if (pos > 0)
	{
		memmove(tcpread.data.buffer,
				tcpread.data.buffer + pos,
				tcpread.data.length - pos);
		
		tcpread.data.length -= pos;
	}
	
	if (TCPConn<bufSize>::state == TCPConn<bufSize>::CONNECTED && running && !tcpread.active)
		IOProcessor::Add(&tcpread);
}

template<int bufSize>
void MessageConn<bufSize>::Stop()
{
	Log_Trace();
	running = false;
}

template<int bufSize>
void MessageConn<bufSize>::Continue()
{
	Log_Trace();
	running = true;
	OnRead();
}

#endif
