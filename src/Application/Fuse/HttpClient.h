#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "Framework/Transport/TCPConn.h"
#include "Application/Keyspace/Protocol/HttpRequest.h"

class HttpClient : public TCPConn<>
{
public:
	void			Init();
	void			GetRequest(const char *url);

private:
	HttpRequest		response;

	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnClose();
	
	int				ParseResponse(char* buf, int len);
	
};

#endif
