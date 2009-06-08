#ifndef MESSAGECONN_H
#define MESSAGECONN_H

#include "TCPConn.h"

template<int bufferSize = MAX_TCP_MESSAGE_SIZE>
class MessageConn : public TCPConn<bufferSize>
{
public:
	MessageConn() { running = true; }

	virtual void	OnMessageRead(const ByteString& msg) = 0;
	virtual void	OnClose() = 0;
	virtual void	OnRead();
	
	virtual void	Stop() { running = false; }
	
	virtual void	Continue() { running = true; OnRead(); }

private:
	bool			running;
};

template<int bufferSize>
void MessageConn<bufferSize>::OnRead()
{
	TCPRead& tcpread = TCPConn<bufferSize>::tcpread;
	unsigned pos, msglength, nread, msgbegin, msgend;
			
	tcpread.requested = IO_READ_ANY;

	pos = 0;

	while(running)
	{
		msglength = strntouint64(tcpread.data.buffer + pos, tcpread.data.length - pos, &nread);
		
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

		OnMessageRead(ByteString(msglength, msglength, tcpread.data.buffer + msgbegin));
		
		pos = msgend;
		
		// if the user called Close() in OnMessageRead()
		if (TCPConn<bufferSize>::state != TCPConn<bufferSize>::CONNECTED)
			return;
		
		if (tcpread.data.length == msgend)
			break;
	}
	
	if (pos > 0)
	{
		memmove(tcpread.data.buffer, tcpread.data.buffer + pos, tcpread.data.length - pos);
		tcpread.data.length -= pos;
	}
	
	if (TCPConn<bufferSize>::state == TCPConn<bufferSize>::CONNECTED && running)
		IOProcessor::Add(&tcpread);
}

#endif
