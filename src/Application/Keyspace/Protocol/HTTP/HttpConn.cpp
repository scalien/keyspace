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
#define PARAMSEP			','

HttpConn::HttpConn()
{
	server = NULL;
}


void HttpConn::Init(KeyspaceDB* kdb_, HttpServer* server_)
{
	TCPConn<>::Init();
	KeyspaceService::Init(kdb_);
	
	server = server_;
	
	request.Init();
	socket.GetEndpoint(endpoint);
	
	// HACK
	headerSent = false;
	closeAfterSend = false;
}


void HttpConn::OnComplete(KeyspaceOp* op, bool final)
{
	if (op->type == KeyspaceOp::GET ||
		op->type == KeyspaceOp::DIRTY_GET ||
		op->type == KeyspaceOp::ADD ||
		op->type == KeyspaceOp::TEST_AND_SET ||
		op->type == KeyspaceOp::REMOVE)
	{
		if (op->status)
			Response(200, op->value.buffer, op->value.length);
		else
			RESPONSE_NOTFOUND;
	}
	else if (op->type == KeyspaceOp::SET)
	{
		if (op->status)
			Response(200, "OK", strlen("OK"));
		else
			Response(200, "Failed", strlen("Failed"));
	}
	else if (op->type == KeyspaceOp::DELETE || op->type == KeyspaceOp::PRUNE
			|| op->type == KeyspaceOp::RENAME)
	{
		if (op->status)
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
		if (op->key.length > 0)
		{
			Write(op->key.buffer, op->key.length, false);
			Write("\n", 1);
		}
	}
	else if (op->type == KeyspaceOp::LISTP || op->type == KeyspaceOp::DIRTY_LISTP)
	{
		if (!headerSent)
		{
			ResponseHeader(200, false, "Content-type: text/plain" CS_CRLF CS_CRLF);
			headerSent = true;
		}
		if (op->key.length > 0)
		{
			Write(op->key.buffer, op->key.length, false);
			Write(":", 1);
			Write(op->value.buffer, op->value.length, false);
			Write("\n", 1);
		}
	}
	else
		ASSERT_FAIL();

	if (final)
	{
		WritePending(); // flush data to TCP socket
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
#define PREFIXFUNC(prefix, func) \
if (strncmp(request.line.uri, prefix, strlen(prefix)) == 0) \
{ \
	params += strlen(prefix); \
	ret = func; \
}

	KeyspaceOp* op;
	op = new KeyspaceOp;
	op->service = this;
	
	char* params;
	
	params = (char*) request.line.uri;
	bool ret;
		
	PREFIXFUNC("/getmaster",			ProcessGetMaster()) else
	PREFIXFUNC("/get?",					ProcessGet(params, op)) else
	PREFIXFUNC("/dirtyget?",			ProcessGet(params, op, true)) else
	PREFIXFUNC("/set?",					ProcessSet(params, op)) else
	PREFIXFUNC("/testandset?",			ProcessTestAndSet(params, op)) else
	PREFIXFUNC("/add?",					ProcessAdd(params, op)) else
	PREFIXFUNC("/rename?",				ProcessRename(params, op)) else
	PREFIXFUNC("/delete?",				ProcessDelete(params, op)) else
	PREFIXFUNC("/remove?",				ProcessRemove(params, op)) else
	PREFIXFUNC("/prune?",				ProcessPrune(params, op)) else
	PREFIXFUNC("/listkeys?",			ProcessList(params, op, false)) else
	PREFIXFUNC("/dirtylistkeys?",		ProcessList(params, op, false, true)) else
	PREFIXFUNC("/listkeyvalues?",		ProcessList(params, op, true)) else
	PREFIXFUNC("/dirtylistkeyvalues?",	ProcessList(params, op, true, true)) else
	PREFIXFUNC("/latency?",				ProcessLatency())
	else
	{
		RESPONSE_NOTFOUND;
		ret = false;
	}
	
	if (ret && !Add(op))
	{
		RESPONSE_FAIL;
		ret = false;
	}


	if (op->IsWrite() && ret)
		ret = kdb->Submit();
	
	if (!ret)
	{
		delete op;
		return -1;
	}
	
	return 0;
}

bool ParseParams(const char* s, unsigned num, ...)
{
	unsigned	length;
	ByteString* bs;
	va_list		ap;	
	
	va_start(ap, num);

	while (*s != '\0' && num > 0)
	{
		bs = va_arg(ap, ByteString*);
		bs->buffer = (char*) s;
		length = 0;
		while(*s != '\0' && *s != PARAMSEP)
		{
			length++;
			s++;
		}
		bs->length = length;
		bs->size = length;
		if (*s == PARAMSEP)
			s++;
		num--;
	}

	va_end(ap);
	
	if (num == 0 && *s == '\0')
		return true;
	else
		return false;
}

#define VALIDATE_KEYLEN(bs) if (bs.length > KEYSPACE_KEY_SIZE) { RESPONSE_FAIL; return false; }
#define VALIDATE_VALLEN(bs) if (bs.length > KEYSPACE_VAL_SIZE) { RESPONSE_FAIL; return false; }

bool HttpConn::ProcessGetMaster()
{
	ByteArray<32> ba;
	if (kdb->IsMaster())
		ba.Writef("%d\n\nI'm the master", kdb->GetMaster());
	else
		ba.Writef("%d\n\nI'm a slave", kdb->GetMaster());
	
	Response(200, ba.buffer, ba.length);
	return false;
}

bool HttpConn::ProcessGet(const char* params, KeyspaceOp* op, bool dirty)
{
	if (dirty)
		op->type = KeyspaceOp::DIRTY_GET;
	else
		op->type = KeyspaceOp::GET;
	
	op->key.Set((char*) params, strlen(params));
	return true;
}

bool HttpConn::ProcessList(const char* params, KeyspaceOp* op, bool p, bool dirty)
{
	ByteString prefix, key, count, offset;
	unsigned nread;

	if (!p)
	{
		if (!dirty)
			op->type = KeyspaceOp::LIST;
		else
			op->type = KeyspaceOp::DIRTY_LIST;
	}
	else
	{
		if (!dirty)
			op->type = KeyspaceOp::LISTP;
		else
			op->type = KeyspaceOp::DIRTY_LISTP;
	}
	
	ParseParams(params, 4, &prefix, &key, &count, &offset);
	
	VALIDATE_KEYLEN(prefix);
	VALIDATE_KEYLEN(key);
	
	op->prefix.Set(prefix);
	op->key.Set(key);
	op->count = strntoint64(count.buffer, count.length, &nread);
	if (nread != (unsigned) count.length)
	{
		RESPONSE_FAIL;
		return false;
	}
	op->offset = strntoint64(offset.buffer, offset.length, &nread);
	if (nread != (unsigned) offset.length)
	{
		RESPONSE_FAIL;
		return false;
	}
	return true;
}

bool HttpConn::ProcessSet(const char* params, KeyspaceOp* op)
{
	ByteString key, value;

	ParseParams(params, 2, &key, &value);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(value);
	
	op->type = KeyspaceOp::SET;
	op->key.Set(key);
	op->value.Set(value);	
	return true;
}

bool HttpConn::ProcessTestAndSet(const char* params, KeyspaceOp* op)
{
	ByteString key, test, value;

	ParseParams(params, 3, &key, &test, &value);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(test);
	VALIDATE_VALLEN(value);
	
	op->type = KeyspaceOp::TEST_AND_SET;
	
	op->key.Set(key);
	op->test.Set(test);
	op->value.Set(value);
	return true;
}

bool HttpConn::ProcessAdd(const char* params, KeyspaceOp* op)
{
	ByteString key, value;
	unsigned nread;

	ParseParams(params, 2, &key, &value);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(value);
	
	op->type = KeyspaceOp::ADD;
	
	op->key.Set(key);
	op->num	= strntoint64(value.buffer, value.length, &nread);
	if (nread != (unsigned) value.length)
	{
		RESPONSE_FAIL;
		return false;
	}
	return true;
}

bool HttpConn::ProcessRename(const char* params, KeyspaceOp* op)
{
	ByteString key, newKey;

	ParseParams(params, 2, &key, &newKey);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_KEYLEN(newKey);
	
	op->type = KeyspaceOp::RENAME;
	op->key.Set(key);
	op->newKey.Set(newKey);
	return true;
}

bool HttpConn::ProcessDelete(const char* params, KeyspaceOp* op)
{
	ByteString key;

	ParseParams(params, 1, &key);
	
	VALIDATE_KEYLEN(key);
	
	op->type = KeyspaceOp::DELETE;
	op->key.Set(key);
	return true;
}

bool HttpConn::ProcessRemove(const char* params, KeyspaceOp* op)
{
	ByteString key;

	ParseParams(params, 1, &key);
	
	VALIDATE_KEYLEN(key);
	
	op->type = KeyspaceOp::REMOVE;
	op->key.Set(key);
	return true;
}

bool HttpConn::ProcessPrune(const char* params, KeyspaceOp* op)
{
	ByteString prefix;

	ParseParams(params, 1, &prefix);
	
	VALIDATE_KEYLEN(prefix);
	
	op->type = KeyspaceOp::PRUNE;
	op->prefix.Set(prefix);
	return true;
}

bool HttpConn::ProcessLatency()
{
	Response(200, "OK", 2);
	return true;
}

const char* HttpConn::Status(int code)
{
	if (code == 200)
		return "OK";
	if (code == 404)
		return "Not found";
	
	return "";
}

void HttpConn::Response(int code, const char* data, int len, bool close, const char* header)
{	
	DynArray<MAX_MESSAGE_SIZE> httpHeader;
	unsigned size;

	Log_Message("[%s] HTTP: %s %s %d %d", endpoint.ToString(), request.line.method, request.line.uri, code, len);

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

	Log_Message("[%s] HTTP: %s %s %d ?", endpoint.ToString(), request.line.method, request.line.uri, code);

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
