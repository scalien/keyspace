#ifndef MESSAGECONN_H
#define MESSAGECONN_H

#include "TCPConn.h"

template<int bufferSize = MAX_TCP_MESSAGE_SIZE>
class MessageConn : public TCPConn<bufferSize>
{
public:
	virtual void	OnMessageRead(const ByteString& msg) = 0;
	virtual void	OnClose() = 0;
	virtual void	OnRead();	
};

template<int bufferSize>
void MessageConn<bufferSize>::OnRead()
{
	TCPRead& tcpread = TCPConn<bufferSize>::tcpread;
	unsigned msglength, nread, msgbegin, msgend;
			
	tcpread.requested = IO_READ_ANY;

	do
	{
		msglength = strntouint64(tcpread.data.buffer, tcpread.data.length, &nread);
		
		if (msglength > tcpread.data.size - NumLen(msglength) - 1)
		{
			OnClose();
			return;
		}
		
		if (nread == 0 || (unsigned) tcpread.data.length <= nread)
			break;
			
		if (tcpread.data.buffer[nread] != ':')
		{
			Log_Trace("Message protocol error");
			OnClose();
			return;
		}
	
		msgbegin = nread + 1;
		msgend = nread + 1 + msglength;
		
		if ((unsigned) tcpread.data.length < msgend)
		{
			tcpread.requested = msgend;
			break;
		}

		OnMessageRead(ByteString(msglength, msglength, tcpread.data.buffer + msgbegin));
		
		// if the user called Close() in OnMessageRead()
		if (TCPConn<bufferSize>::state != TCPConn<bufferSize>::CONNECTED)
			return;
		
		// move the rest back to the beginning of the buffer
		memcpy(tcpread.data.buffer, tcpread.data.buffer + msgend,
			tcpread.data.length - msgend);
		tcpread.data.length = tcpread.data.length - msgend;
		
		if (tcpread.data.length == 0)
			break;
	}
	while (true);
	
	if (TCPConn<bufferSize>::state == TCPConn<bufferSize>::CONNECTED)
		IOProcessor::Add(&tcpread);
}


#endif
