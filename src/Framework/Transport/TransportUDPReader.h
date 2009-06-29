#ifndef TRANSPORTUDPREADER_H
#define TRANSPORTUDPREADER_H

#include "System/IO/Socket.h"
#include "Transport.h"
#include "TransportReader.h"

class TransportUDPReader : public TransportReader
{
typedef ByteArray<MAX_UDP_MESSAGE_SIZE> Buffer;
typedef MFunc<TransportUDPReader> Func;
public:
	TransportUDPReader();
	
	bool			Init(int port);

	void			SetOnRead(Callable* onRead);
	void			GetMessage(ByteString& bs_);
	void			GetEndpoint(Endpoint& endpoint);
	void			Stop();
	void			Continue();
	bool			IsActive();
	void			OnRead();
	
private:
	Socket			socket;
	UDPRead			udpread;
	Buffer			data;
	Func			onRead;
	Callable*		userCallback;
};

#endif
