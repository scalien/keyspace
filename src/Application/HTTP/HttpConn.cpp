// TODOs
// - Add API key

#include "HttpConn.h"
#include "Version.h"
#include "HttpServer.h"
#include "HttpConsts.h"
#include "Framework/Database/Database.h"

#define MAX_MESSAGE_SIZE	4096
#define CS_CR				"\015"
#define CS_LF				"\012"
#define CS_CRLF				CS_CR CS_LF
#define MSG_FAIL			"Unable to process your request at this time"
#define MSG_NOT_FOUND		"Not found"
#define PARAMSEP			','

HttpConn::HttpConn()
{
	server = NULL;
}


void HttpConn::Init(HttpServer* server_)
{
	TCPConn<>::Init();
	
	onCloseCallback = NULL;
	server = server_;
	
	request.Init();
	socket.GetEndpoint(endpoint);
	
	// HACK
	closeAfterSend = false;
}

void HttpConn::SetOnClose(Callable* callable)
{
	onCloseCallback = callable;
}

void HttpConn::OnRead()
{
	Log_Trace();
	int len;
	
	len = Parse(tcpread.data.buffer, tcpread.data.length);

	if (len < 0)
		IOProcessor::Add(&tcpread);
}


void HttpConn::OnClose()
{
	Log_Trace();
	
	Close();
	request.Free();
	if (closeAfterSend)
	{
		if (onCloseCallback)
			Call(onCloseCallback);
		else
			server->DeleteConn(this);
	}
}


void HttpConn::OnWrite()
{
	Log_Trace();
	TCPConn<>::OnWrite();
	if (closeAfterSend && !tcpwrite.active)
		OnClose();
}


void HttpConn::Print(const char* s)
{
	Write(s, strlen(s), false);
}


int HttpConn::Parse(char* buf, int len)
{
	int pos;
	
	pos = request.Parse(buf, len);
	if (pos <= 0)
		return pos;
	
	if (server->HandleRequest(this, request))
		return -1;
	
	Response(HTTP_STATUS_CODE_NOT_FOUND, MSG_NOT_FOUND, sizeof(MSG_NOT_FOUND) - 1);
	closeAfterSend = true;
		
	return -1;
}


const char* HttpConn::Status(int code)
{
	if (code == HTTP_STATUS_CODE_OK)
		return "OK";
	if (code == HTTP_STATUS_CODE_NOT_FOUND)
		return "Not found";
	
	return "";
}

void HttpConn::Response(int code, const char* data,
int len, bool close, const char* header)
{	
	DynArray<MAX_MESSAGE_SIZE> httpHeader;
	unsigned size;

	Log_Message("[%s] HTTP: %s %s %d %d", endpoint.ToString(),
				request.line.method, request.line.uri, code, len);

	do {
		size = snwritef(httpHeader.buffer, httpHeader.size,
					"%s %d %s" CS_CRLF
					"Accept-Range: bytes" CS_CRLF
					"Content-Length: %d" CS_CRLF
					"Cache-Control: no-cache" CS_CRLF
					"%s"
					"%s"
					CS_CRLF
					, 
					request.line.version, code, Status(code),
					len,
					close ? "Connection: close" CS_CRLF : "",
					header ? header : "");

		if (size <= httpHeader.size)
			break;

		httpHeader.Reallocate(size, false);
	} while (1);
			
	Write(httpHeader.buffer, size, false);
	Write(data, len);
}

void HttpConn::ResponseHeader(int code, bool close, const char* header)
{
	DynArray<MAX_MESSAGE_SIZE> httpHeader;
	unsigned size;

	Log_Message("[%s] HTTP: %s %s %d ?",
				endpoint.ToString(), request.line.method,
				request.line.uri, code);

	do {
		size = snwritef(httpHeader.buffer, httpHeader.size,
					"%s %d %s" CS_CRLF
					"Cache-Control: no-cache" CS_CRLF
					"%s"
					"%s"
					CS_CRLF
					, 
					request.line.version, code, Status(code),
					close ? "Connection: close" CS_CRLF : "",
					header ? header : "");

		if (size <= httpHeader.size)
			break;

		httpHeader.Reallocate(size, false);
	} while (1);
			
	Write(httpHeader.buffer, size, false);
}

void HttpConn::Flush(bool closeAfterSend_)
{
	WritePending();
	closeAfterSend = closeAfterSend_;
}
