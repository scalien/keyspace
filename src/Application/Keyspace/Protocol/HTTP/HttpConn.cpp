#include "HttpConn.h"
#include "Version.h"
#include "HttpServer.h"

#define MAX_MESSAGE_SIZE	4096
#define CS_CR				"\015"
#define CS_LF				"\012"
#define CS_CRLF				CS_CR CS_LF
#define MSG_FAIL			"Unable to process your request at this time"
#define MSG_NOT_FOUND		"Not found"
#define RESPONSE_FAIL		Response(500, MSG_FAIL, sizeof(MSG_FAIL) - 1)
#define RESPONSE_NOTFOUND	Response(404, MSG_NOT_FOUND,\
							sizeof(MSG_NOT_FOUND) - 1)
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
	static bool rowp = 0;
	if (op->type == KeyspaceOp::GET ||
		op->type == KeyspaceOp::DIRTY_GET ||
		op->type == KeyspaceOp::ADD ||
		op->type == KeyspaceOp::TEST_AND_SET ||
		op->type == KeyspaceOp::REMOVE)
	{
		if (op->status)
		{
			if (html)
			{
				ResponseHeader(200, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				Print("<title>");
				Print("Keyspace contents of: ");
				Write(op->key.buffer, op->key.length, false);
				Print("</title>");
				Write(op->value.buffer, op->value.length, true);
			}
			else
				Response(200, op->value.buffer, op->value.length);
		}
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
	else if (op->type == KeyspaceOp::DELETE ||
			 op->type == KeyspaceOp::PRUNE ||
			 op->type == KeyspaceOp::RENAME)
	{
		if (op->status)
			Response(200, "OK", strlen("OK"));
		else
			RESPONSE_NOTFOUND;
	}
	else if (op->type == KeyspaceOp::LIST ||
			 op->type == KeyspaceOp::DIRTY_LIST)
	{
		if (!headerSent)
		{
			if (html)
			{
				ResponseHeader(200, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				Print("<title>");
				Print("Keyspace listing of: ");
				Write(op->prefix.buffer, op->prefix.length, false);
				Print("</title>");
				PrintStyle();
			}
			else
			{
				ResponseHeader(200, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
			}
			headerSent = true;
		}
		if (op->key.length > 0)
		{
			if (html)
			{
				if (rowp)
					Print("<div class='even'>");
				else
					Print("<div class='odd'>");
				if (op->type == KeyspaceOp::LIST)
					Print("<a href='/html/get?");
				else
					Print("<a href='/html/dirtyget?");
				Write(op->key.buffer, op->key.length, false);
				Print("'>");
				Write(op->key.buffer, op->key.length, false);
				Print("</a></div>");
				rowp = !rowp;
			}
			else
			{
				Write(op->key.buffer, op->key.length, false);
				Print("\n");
			}
		}
	}
	else if (op->type == KeyspaceOp::LISTP ||
			 op->type == KeyspaceOp::DIRTY_LISTP)
	{
		if (!headerSent)
		{
			if (html)
			{
				ResponseHeader(200, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				Print("<title>");
				Print("Keyspace listing of: ");
				Write(op->prefix.buffer, op->prefix.length, false);
				Print("</title>");
				PrintStyle();
			}
			else
			{
				ResponseHeader(200, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
			}
			headerSent = true;
		}
		if (op->key.length > 0)
		{
			if (html)
			{
				if (rowp)
					Print("<div class='even'>");
				else
					Print("<div class='odd'>");
				if (op->type == KeyspaceOp::LIST)
					Print("<a href='/html/get?");
				else
					Print("<a href='/html/dirtyget?");
				Write(op->key.buffer, op->key.length, false);
				Print("'>");
				Write(op->key.buffer, op->key.length, false);
				Print("</a>");
				Print(" => ");
				Print("<span class='value'>");
				Write(op->value.buffer, op->value.length, false);
				Print("</span>");
				Print("</div>");
				rowp = !rowp;
			}
			else
			{
				Write(op->key.buffer, op->key.length, false);
				Print(" => ");
				Write(op->value.buffer, op->value.length, false);
				Print("\n");
			}
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


bool HttpConn::IsAborted()
{
	if (state == DISCONNECTED)
		return true;
	return false;
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


void HttpConn::Print(const char* s)
{
	Write(s, strlen(s));
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
#define MF(prefix, func) \
if (strncmp(request.line.uri, prefix, strlen(prefix)) == 0) \
{ \
	pars += strlen(prefix); \
	ret = func; \
}

	KeyspaceOp* op;
	op = new KeyspaceOp;
	op->service = this;
	
	char* pars;
	
	pars = (char*) request.line.uri;
	bool ret;
		
	MF("/get?",					ProcessGet(pars, op, false, false)) else
	MF("/dirtyget?",			ProcessGet(pars, op, true, false)) else
	MF("/set?",					ProcessSet(pars, op)) else
	MF("/testandset?",			ProcessTestAndSet(pars, op)) else
	MF("/add?",					ProcessAdd(pars, op)) else
	MF("/rename?",				ProcessRename(pars, op)) else
	MF("/delete?",				ProcessDelete(pars, op)) else
	MF("/remove?",				ProcessRemove(pars, op)) else
	MF("/prune?",				ProcessPrune(pars, op)) else
	MF("/listkeys?",			ProcessList(pars, op, false, false, false)) else
	MF("/dirtylistkeys?",		ProcessList(pars, op, false, true, false)) else
	MF("/listkeyvalues?",		ProcessList(pars, op, true, false, false)) else
	MF("/dirtylistkeyvalues?",	ProcessList(pars, op, true, true, false)) else
	MF("/html/get?",			ProcessGet(pars, op, false, true)) else
	MF("/html/dirtyget?",		ProcessGet(pars, op, true, true)) else
	MF("/html/dirtylistkeys?",	ProcessList(pars, op, false, true, true)) else
	MF("/html/listkeyvalues?",	ProcessList(pars, op, true, false, true)) else
	MF("/html/listkeys?",		ProcessList(pars, op, false, false, true)) else
	MF("/html/dirtylistkeys?",	ProcessList(pars, op, false, true, true)) else
	MF("/latency?",				ProcessLatency()) else
	if (request.line.uri[0] == 0 ||
		(request.line.uri[0] == '/' && request.line.uri[1] == 0))
	{
		ret = PrintHello();
		ret = false;
	}
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

#define VALIDATE_KEYLEN(bs)\
if (bs.length > KEYSPACE_KEY_SIZE) { RESPONSE_FAIL; return false; }

#define VALIDATE_VALLEN(bs)\
if (bs.length > KEYSPACE_VAL_SIZE) { RESPONSE_FAIL; return false; }

bool HttpConn::ProcessGet(const char* params, KeyspaceOp* op,
bool dirty, bool html_)
{
	html = html_;
	
	if (dirty)
		op->type = KeyspaceOp::DIRTY_GET;
	else
		op->type = KeyspaceOp::GET;
	
	op->key.Set((char*) params, strlen(params));
	return true;
}

bool HttpConn::ProcessList(const char* params,
KeyspaceOp* op, bool p, bool dirty, bool html_)
{
	ByteString prefix, key, count, offset, direction;
	unsigned nread;

	html = html_;

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
	
	ParseParams(params, 5, &prefix, &key, &count, &offset, &direction);
	
	VALIDATE_KEYLEN(prefix);
	VALIDATE_KEYLEN(key);
	
	op->prefix.Set(prefix);
	op->key.Set(key);
	op->count = strntoint64(count.buffer, count.length, &nread);
	if (direction.length != 1)
		op->forward = true;
	else
		op->forward = (direction.buffer[0] == 'f');
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

bool HttpConn::PrintHello()
{
	ByteArray<128> text;
	text.length = snprintf(text.buffer, text.size,
		"Keyspace v" VERSION_STRING " r%.*s running\n\nMaster: %d%s%s",
		(int)VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER,
		kdb->GetMaster(),
		kdb->IsMaster() ? " (me)" : "",
		kdb->IsMasterKnown() ? "" : " (unknown)");

	Response(200, text.buffer, text.length);
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

void HttpConn::PrintStyle()
{
	Print("<style>");
	Print("div { padding: 4px; ");
	Print("      font-size: 12px; font-family: courier; }");
	Print("div.even { background-color: white; }");
	Print("div.odd { background-color: #F0F0F0; }");
	Print("a:link { text-decoration:none; }");
	Print("a:visited { text-decoration:none; }");
	Print("a:hover { text-decoration: underline; }");
	Print("span.value { color: #006600; }");
	Print("</style>");
}
