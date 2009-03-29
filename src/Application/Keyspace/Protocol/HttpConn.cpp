#include "HttpConn.h"
#include "HttpServer.h"

#define MAX_MESSAGE_SIZE	4096
#define CS_CR				"\015"
#define CS_LF				"\012"
#define CS_CRLF				CS_CR CS_LF

HttpConn::HttpConn()
{
	server = NULL;
}


void HttpConn::Init(KeyspaceDB* kdb_, HttpServer* server_)
{
	TCPConn<>::Init();

	kdb = kdb_;
	server = server_;
	
	request.Init();
	buffer = readBuffer.buffer;
	offs = 0;
}


void HttpConn::OnComplete(KeyspaceOp* op, bool status)
{
	Log_Trace();
	
	if (op->type == KeyspaceOp::GET)
	{
		if (status)
			Response(200, op->value.buffer, op->value.length);
		else
			Response(404, "Not found", strlen("Not found"));
	}
	else if (op->type == KeyspaceOp::SET || op->type == KeyspaceOp::TEST_AND_SET)
	{
		if (status)
			Response(200, "OK", strlen("OK"));
		else
			Response(200, "Failed", strlen("Failed"));
	}
}


void HttpConn::OnRead()
{
	Log_Trace();
	
	Parse(tcpread.data.buffer, tcpread.data.length);
	
	IOProcessor::Get()->Add(&tcpread);
}


void HttpConn::OnClose()
{
	Log_Trace();
	Close();
	server->DeleteConn(this);
}


int HttpConn::Parse(char* buf, int len)
{
	int pos;
	int ret;
	
	pos = request.Parse(buf, len);
	if (pos <= 0)
		return pos;
	
	if (strcmp(request.line.method, "GET") == 0)
	{
		ret = ProcessGetRequest();
		if (ret < 0)
			return -1;
		
		return pos;
	}
	
	return -1;
}

int HttpConn::ProcessGetRequest()
{
	KeyspaceOp op;
	const char* key;
	const char* value;
	const char* test;
	int keylen;
	int valuelen;
	int testlen;
	// http://localhost:8080/get/key
	// http://localhost:8080/set/key/value
	
	if (strncmp(request.line.uri, "/get/", strlen("/get/")) == 0)
	{
		key = request.line.uri + strlen("/get/");
		keylen = strlen(key);
		
		op.client = this;
		op.type = KeyspaceOp::GET;
		op.key.buffer = (char*) key;
		op.key.length = keylen;
		op.key.size = keylen;
		
		op.value.Init();
		op.test.Init();
		
		if (!kdb->Add(op))
			Response(500, "Unable to process your request at this time",
				strlen("Unable to process your request at this time"));
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/set/", strlen("/set/")) == 0)
	{
		key = request.line.uri + strlen("/set/");
		value = strchr(key, '/');
		if (!value)
			return -1;
		
		keylen = value - key;
		value++;
		valuelen = strlen(value);
		
		op.client = this;
		op.type = KeyspaceOp::SET;
		op.key.buffer = (char*) key;
		op.key.length = keylen;
		op.key.size = keylen;
		
		op.value.buffer = (char*) value;
		op.value.length = valuelen;
		op.value.size = valuelen;
		
		op.test.Init();
		
		if (!kdb->Add(op))
			Response(500, "Unable to process your request at this time",
				strlen("Unable to process your request at this time"));
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/testandset/", strlen("testandset")) == 0)
	{
		key = request.line.uri + strlen("/testandset/");
		test = strchr(key, '/');
		if (!test)
			return -1;
		
		keylen = test - key;
		test++;
		
		value = strchr(test, '/');
		if (!value)
			return -1;
		
		testlen = value - test;
		value++;
		
		valuelen = strlen(value);
		
		op.client = this;
		op.type = KeyspaceOp::TEST_AND_SET;
		op.key.buffer = (char*) key;
		op.key.length = keylen;
		op.key.size = keylen;
		
		op.test.buffer = (char*) test;
		op.test.length = testlen;
		op.test.size = testlen;
		
		op.value.buffer = (char*) value;
		op.value.length = valuelen;
		op.value.size = valuelen;
		
		if (!kdb->Add(op))
			Response(500, "Unable to process your request at this time",
				strlen("Unable to process your request at this time"));
		
		return 0;
	}

	else if (strcmp(request.line.uri, "/latency") == 0)
	{
		Response(200, "OK", 2);
		return 0;
	}
	else
	{
		// 404
		Response(404, "Not found", 9);
	}
	
	return -1;
}

const char* HttpConn::Status(int code)
{
	if (code == 200)
		return "OK";
	if (code == 404)
		return "Not found";
	
	return "";
}

void HttpConn::Response(int code, char* data, int len, bool close, char* header)
{	
	char buf[MAX_MESSAGE_SIZE];	// TODO can be bigger than that
	const char *p;
	int size;
	
	size = snprintf(buf, sizeof(buf), 
					"%s %d %s" CS_CRLF
					"Accept-Range: bytes" CS_CRLF
					"Content-Length: %d" CS_CRLF
					"Cache-Control: no-cache" CS_CRLF
					"%s"
					"%s"
					CS_CRLF
					"%s"
					, 
					request.line.version, code, Status(code),
					len,
					close ? "Connection: close" CS_CRLF : "",
					header ? header : "",
					data);
		
	p = buf;
	
	Write(p, size);
	
}
