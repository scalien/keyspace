#include "HttpConn.h"
#include "HttpServer.h"

#define MAX_MESSAGE_SIZE	4096
#define CS_CR				"\015"
#define CS_LF				"\012"
#define CS_CRLF				CS_CR CS_LF
#define MSG_FAIL			"Unable to process your request at this time"
#define MSG_NOT_FOUND		"Not found"
#define RESPONSE_FAIL		Response(500, MSG_FAIL, sizeof(MSG_FAIL) - 1)
#define RESPONSE_NOTFOUND	Response(404, MSG_NOT_FOUND, sizeof(MSG_NOT_FOUND) - 1)


HttpConn::HttpConn()
{
	server = NULL;
}


void HttpConn::Init(KeyspaceDB* kdb_, HttpServer* server_)
{
	TCPConn<>::Init();
	KeyspaceClient::Init(kdb_);
	
	server = server_;
	
	request.Init();
	
	// HACK
	headerSent = false;
	closeAfterSend = false;
}


void HttpConn::OnComplete(KeyspaceOp* op, bool status, bool final)
{
	Log_Trace();
		
	if (op->type == KeyspaceOp::GET ||
		op->type == KeyspaceOp::DIRTY_GET ||
		op->type == KeyspaceOp::ADD)
	{
		if (status)
			Response(200, op->value.buffer, op->value.length);
		else
			RESPONSE_NOTFOUND;
	}
	else if (op->type == KeyspaceOp::SET ||
			 op->type == KeyspaceOp::TEST_AND_SET)
	{
		if (status)
			Response(200, "OK", strlen("OK"));
		else
			Response(200, "Failed", strlen("Failed"));
	}
	else if (op->type == KeyspaceOp::DELETE)
	{
		if (status)
			Response(200, "OK", strlen("OK"));
		else
			RESPONSE_NOTFOUND;
	}
	else if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
	{
		if (!headerSent)
		{
			ResponseHeader(200, false, "Content-type: text/plain" CS_CRLF CS_CRLF);
			headerSent = true;
		}
		Write(op->key.buffer, op->key.length, false);
		Write("\n", 1);
	}
	else
		ASSERT_FAIL();

	if (final)
	{
		numpending--;
		delete op;
		closeAfterSend = true;
	}

	if (state == DISCONNECTED && numpending == 0)
		server->DeleteConn(this);
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
	if (numpending == 0)
		server->DeleteConn(this);
}


void HttpConn::OnWrite()
{
	Log_Trace();
	TCPConn<>::OnWrite();
	if (closeAfterSend && !tcpwrite.active)
		OnClose();
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
	KeyspaceOp* op;
	const char* key;
	const char* value;
	const char* test;
	const char* prefix;
	const char* count;
	int keylen;
	int valuelen;
	int testlen;
	int prefixlen;
	int countlen;
	unsigned nread;
	// http://localhost:8080/get/key
	// http://localhost:8080/set/key/value
	
	op = new KeyspaceOp;
	op->client = this;
	
	if (strncmp(request.line.uri, "/get/", strlen("/get/")) == 0)
	{
		key = request.line.uri + strlen("/get/");
		keylen = strlen(key);
		
		if (keylen > KEYSPACE_KEY_SIZE)
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		
		op->type = KeyspaceOp::GET;
		
		if (!op->key.Allocate(keylen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->key.Set((char*) key, keylen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/dirtyget/", strlen("/dirtyget/")) == 0)
	{
		key = request.line.uri + strlen("/dirtyget/");
		keylen = strlen(key);
		
		if (keylen > KEYSPACE_KEY_SIZE)
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		
		op->type = KeyspaceOp::DIRTY_GET;
		
		if (!op->key.Allocate(keylen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->key.Set((char*) key, keylen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/set/", strlen("/set/")) == 0)
	{
		key = request.line.uri + strlen("/set/");
		value = strchr(key, '/');
		if (!value)
		{
			delete op;
			return -1;
		}
		
		keylen = value - key;
		value++;
		valuelen = strlen(value);
		
		if (keylen > KEYSPACE_KEY_SIZE || valuelen > KEYSPACE_VAL_SIZE)
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		
		op->type = KeyspaceOp::SET;
		
		if (!op->key.Allocate(keylen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->key.Set((char*) key, keylen);
		
		if (!op->value.Allocate(valuelen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->value.Set((char*) value, valuelen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/testandset/", strlen("testandset")) == 0)
	{
		key = request.line.uri + strlen("/testandset/");
		test = strchr(key, '/');
		if (!test)
		{
			delete op;
			return -1;
		}
		
		keylen = test - key;
		test++;
		
		value = strchr(test, '/');
		if (!value)
		{
			delete op;
			return -1;
		}
		
		testlen = value - test;
		value++;
		
		valuelen = strlen(value);
		
		if (keylen > KEYSPACE_KEY_SIZE || testlen > KEYSPACE_VAL_SIZE || valuelen > KEYSPACE_VAL_SIZE)
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		
		op->type = KeyspaceOp::TEST_AND_SET;
		
		if (!op->key.Allocate(keylen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->key.Set((char*) key, keylen);
		
		if (!op->test.Allocate(testlen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->test.Set((char*) test, testlen);
		
		if (!op->value.Allocate(valuelen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->value.Set((char*) value, valuelen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/add/", strlen("/add/")) == 0)
	{
		key = request.line.uri + strlen("/add/");
		value = strchr(key, '/');
		if (!value)
		{
			delete op;
			return -1;
		}
		
		keylen = value - key;
		value++;
		valuelen = strlen(value);
		
		if (keylen > KEYSPACE_KEY_SIZE || valuelen > KEYSPACE_VAL_SIZE)
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		
		if (valuelen < 1)
		{
			delete op;
			RESPONSE_FAIL;
			return -1;
		}
		
		op->num = strntoint64_t(value, valuelen, &nread);
		if (nread != (unsigned) valuelen)
		{
			delete op;
			RESPONSE_FAIL;
			return -1;
		}
		
		op->type = KeyspaceOp::ADD;
		
		if (!op->key.Allocate(keylen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->key.Set((char*) key, keylen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/delete/", strlen("/delete/")) == 0)
	{
		key = request.line.uri + strlen("/delete/");
		keylen = strlen(key);
		
		if (keylen > KEYSPACE_KEY_SIZE)
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		
		op->type = KeyspaceOp::DELETE;
		
		if (!op->key.Allocate(keylen))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->key.Set((char*) key, keylen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
		
		return 0;
	}
	else if (strncmp(request.line.uri, "/list/", strlen("/list/")) == 0)
	{
		prefix = request.line.uri + strlen("/list/");
		
		op->type = KeyspaceOp::LIST;
		
		count = strchr(prefix, '/');
		if (count == NULL)
		{
			prefixlen = strlen(prefix);
			op->count = 0;
		}
		else
		{
			prefixlen = count - prefix;
			
			if (prefixlen > KEYSPACE_KEY_SIZE)
			{
				delete op;
				RESPONSE_FAIL;
				return 0;
			}
			
			count++;
			countlen = strlen(count);
			
			if (countlen < 1)
			{
				op->count = 0;
			}
			else
			{
				op->count = strntouint64_t(count, countlen, &nread);
				if (nread != (unsigned) countlen)
				{
					delete op;
					RESPONSE_FAIL;
					return 0;
				}
			}
		}
				
		// HACK +1 is to handle empty string (solution is to use DynBuffer)
		if (!op->prefix.Allocate(prefixlen + 1))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->prefix.Set((char*) prefix, prefixlen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
			
		return 0;
	}
	else if (strncmp(request.line.uri, "/dirtylist/", strlen("/dirtylist/")) == 0)
	{
		prefix = request.line.uri + strlen("/dirtylist/");
		
		op->type = KeyspaceOp::DIRTY_LIST;
		
		count = strchr(prefix, '/');
		if (count == NULL)
		{
			prefixlen = strlen(prefix);
			op->count = 0;
		}
		else
		{
			prefixlen = count - prefix;
			
			if (prefixlen > KEYSPACE_KEY_SIZE)
			{
				delete op;
				RESPONSE_FAIL;
				return 0;
			}
			
			count++;
			countlen = strlen(count);
			
			if (countlen < 1)
			{
				op->count = 0;
			}
			else
			{
				op->count = strntouint64_t(count, countlen, &nread);
				if (nread != (unsigned) countlen)
				{
					delete op;
					RESPONSE_FAIL;
					return 0;
				}
			}
		}
				
		// HACK +1 is to handle empty string (solution is to use DynBuffer)
		if (!op->prefix.Allocate(prefixlen + 1))
		{
			delete op;
			RESPONSE_FAIL;
			return 0;
		}
		op->prefix.Set((char*) prefix, prefixlen);
		
		if (!Add(op))
		{
			delete op;
			RESPONSE_FAIL;
		}
			
		return 0;
	}
	else if (strcmp(request.line.uri, "/latency") == 0)
	{
		delete op;
		Response(200, "OK", 2);
		return 0;
	}
	else
	{
		delete op;
		RESPONSE_NOTFOUND;
		return 0;
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
	DynArray<MAX_MESSAGE_SIZE> httpHeader;
	int size;

	do {
		size = snprintf(httpHeader.buffer, httpHeader.size,
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

void HttpConn::ResponseHeader(int code, bool close, char* header)
{
	DynArray<MAX_MESSAGE_SIZE> httpHeader;
	int size;

	do {
		size = snprintf(httpHeader.buffer, httpHeader.size,
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
