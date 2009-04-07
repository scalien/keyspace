#include "HttpClient.h"

#define CR					13
#define LF					10
#define CS_CR				"\015"
#define CS_LF				"\012"
#define CS_CRLF				CS_CR CS_LF

void HttpClient::Init()
{
	TCPConn<>::Init();
	response.Init();
}

void HttpClient::GetRequest(const char* url)
{
	char* hostname = NULL;
	
	Write("GET ", 4, false);
	Write(url, strlen(url), false);
	Write(" HTTP/1.1" CS_CRLF, 11, false);
	Write("Host: ", 6, false);
	if (hostname)
		Write(hostname, strlen(hostname), false);
	else
		Write("localhost", sizeof("localhost") - 1);
	Write(CS_CRLF CS_CRLF, 4, false);
}

void HttpClient::OnRead()
{
}

void HttpClient::OnClose()
{
}

int HttpClient::ParseResponse(char* buf, int len)
{
	return 0;
}
