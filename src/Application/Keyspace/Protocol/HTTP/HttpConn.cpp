// TODOs
// - Add API key

#include "HttpConn.h"
#include "Version.h"
#include "HttpServer.h"
#include "Framework/Database/Database.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"

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

	type = PLAIN;
	rowp = false;
	jsonCallback.Init();
}


void HttpConn::OnComplete(KeyspaceOp* op, bool final)
{
	if (op->type == KeyspaceOp::GET ||
		op->type == KeyspaceOp::DIRTY_GET ||
		op->type == KeyspaceOp::COUNT ||
		op->type == KeyspaceOp::DIRTY_COUNT ||
		op->type == KeyspaceOp::ADD ||
		op->type == KeyspaceOp::TEST_AND_SET ||
		op->type == KeyspaceOp::REMOVE)
	{
		if (op->status)
		{
			if (type == HTML)
			{
				ResponseHeader(200, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				Print("<title>");
				Print("Keyspace contents of: ");
				Write(op->key.buffer, op->key.length, false);
				Print("</title>");
				Write(op->value.buffer, op->value.length, true);
			}
			else if (type == JSON)
			{
				ResponseHeader(200, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
				if (jsonCallback.length)
				{
					Write(jsonCallback.buffer, jsonCallback.length, false);
					Print("(");
				}
				Print("{");
				PrintJSONString(op->key.buffer, op->key.length);
				Print(":");
				PrintJSONString(op->value.buffer, op->value.length);
				Print("}");
				if (jsonCallback.length)
					Print(")");
			}
			else
				Response(200, op->value.buffer, op->value.length);
		}
		else
		{
			if (type == JSON)
				ResponseFail();
			else
				ResponseNotFound();
		}
	}
	else if (op->type == KeyspaceOp::SET)
	{
		if (op->status)
			if (type == JSON)
				PrintJSONStatus("ok");
			else
				Response(200, "OK", strlen("OK"));
		else
			if (type == JSON)
				PrintJSONStatus("failed");
			else
				Response(200, "Failed", strlen("Failed"));
	}
	else if (op->type == KeyspaceOp::DELETE ||
			 op->type == KeyspaceOp::PRUNE ||
			 op->type == KeyspaceOp::RENAME)
	{
		if (op->status)
			if (type == JSON)
				PrintJSONStatus("ok");
			else
				Response(200, "OK", strlen("OK"));
		else
			if (type == JSON)
				PrintJSONStatus("failed");
			else
				ResponseNotFound();
	}
	else if (op->type == KeyspaceOp::LIST ||
			 op->type == KeyspaceOp::DIRTY_LIST)
	{
		if (!headerSent)
		{
			if (type == HTML)
			{
				ResponseHeader(200, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				Print("<title>");
				Print("Keyspace listing of: ");
				Write(op->prefix.buffer, op->prefix.length, false);
				Print("</title>");
				PrintStyle();
			}
			else if (type == JSON)
			{
				ResponseHeader(200, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
				if (jsonCallback.length)
				{
					Write(jsonCallback.buffer, jsonCallback.length, false);
					Print("(");
				}
				Print("{");
				PrintJSONString(op->prefix.buffer, op->prefix.length);
				Print(":[");
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
			if (type == HTML)
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
			else if (type == JSON)
			{
				// with JSON rowp indicates that it is the first item or not
				if (rowp)
					Print(",");
				else
					rowp = true;

				PrintJSONString(op->key.buffer, op->key.length);
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
			if (type == HTML)
			{
				ResponseHeader(200, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				Print("<title>");
				Print("Keyspace listing of: ");
				Write(op->prefix.buffer, op->prefix.length, false);
				Print("</title>");
				PrintStyle();
			}
			else if (type == JSON)
			{
				ResponseHeader(200, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
				if (jsonCallback.length)
				{
					Write(jsonCallback.buffer, jsonCallback.length, false);
					Print("(");
				}
				Print("{");
				PrintJSONString(op->prefix.buffer, op->prefix.length);
				Print(":{");
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
			if (type == HTML)
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
			else if (type == JSON)
			{
				// with JSON rowp indicates that it is the first item or not
				if (rowp)
					Print(",");
				else
					rowp = true;

				PrintJSONString(op->key.buffer, op->key.length);
				Print(":");
				PrintJSONString(op->value.buffer, op->value.length);
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
		if (type == JSON && op->IsList())
		{
			if (op->type == KeyspaceOp::LISTP ||
				op->type == KeyspaceOp::DIRTY_LISTP)
			{
				Print("}");
			}
			else
				Print("]");
			Print("}");
			if (jsonCallback.length)
				Print(")");
		}
		
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
	Write(s, strlen(s), false);
}


void HttpConn::PrintJSONString(const char *s, unsigned len)
{
	Write("\"", 1, false);
	for (unsigned i = 0; i < len; i++)
	{
		if (s[i] == '"')
			Write("\\", 1, false);
		Write(s + i, 1, false);
	}
	Write("\"", 1, false);
}


void HttpConn::PrintJSONStatus(const char* status, const char* type_)
{
	ResponseHeader(200, false,
	"Content-type: text/plain" CS_CRLF CS_CRLF);
	if (jsonCallback.length)
	{
		Write(jsonCallback.buffer, jsonCallback.length, false);
		Print("(");
	}
	Print("{\"status\":\"");
	Print(status);
	if (type_)
	{
		Print("\",\"type\":\"");
		Print(type_);
	}
	Print("\"}");

	if (jsonCallback.length)
		Print(")");				
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

const char* ParseParamsVarArg(const char* s, char sep, unsigned num, va_list ap)
{
	unsigned	length;
	ByteString* bs;

	while (*s != '\0' && num > 0)
	{
		bs = va_arg(ap, ByteString*);
		bs->buffer = (char*) s;
		length = 0;
		while(*s != '\0' && *s != sep)
		{
			length++;
			s++;
		}
		bs->length = length;
		bs->size = length;
		if (*s == sep)
			s++;
		num--;
	}
	
	return s;
}

bool ParseParamsSep(const char* s, char sep, unsigned num, ...)
{
	va_list		ap;	
	
	va_start(ap, num);
	s = ParseParamsVarArg(s, sep, num, ap);
	va_end(ap);
	
	if (num == 0 && *s == '\0')
		return true;
	else
		return false;
}

bool ParseParams(const char* s, unsigned num, ...)
{
	va_list		ap;	
	
	va_start(ap, num);
	s = ParseParamsVarArg(s, PARAMSEP, num, ap);
	va_end(ap);
	
	if (num == 0 && *s == '\0')
		return true;
	else
		return false;
}

int HttpConn::ProcessGetRequest()
{
#define IF_PREFIX(prefix, func) \
if (ret && strncmp(pars, prefix, strlen(prefix)) == 0) \
{ \
	pars += strlen(prefix); \
	ret = func; \
}

	KeyspaceOp* op;
	op = new KeyspaceOp;
	op->service = this;
	
	// note that these variables are used in IF_PREFIX macro
	char* pars = (char*) request.line.uri;
	bool ret = true;

	// first see if the url contains the response type (json or html)
	type = PLAIN;
	IF_PREFIX("/admin/checkpoint", (ProcessAdminCheckpoint(), true)) else
	IF_PREFIX("/json", (type = JSON, true)) else
	IF_PREFIX("/html", (type = HTML, true));
	
	// special case for JSONP callback
	if (type == JSON && *pars == PARAMSEP)
	{
		pars++;
		ParseParamsSep(pars, '/', 1, &jsonCallback);
		if (jsonCallback.length == 0)
			ret = false;
		else
			pars += jsonCallback.length;
	}
	
	// parse the command part of the request uri
	IF_PREFIX("/get?",					ProcessGet(pars, op, false)) else
	IF_PREFIX("/dirtyget?",				ProcessGet(pars, op, true)) else
	IF_PREFIX("/set?",					ProcessSet(pars, op)) else
	IF_PREFIX("/testandset?",			ProcessTestAndSet(pars, op)) else
	IF_PREFIX("/add?",					ProcessAdd(pars, op)) else
	IF_PREFIX("/rename?",				ProcessRename(pars, op)) else
	IF_PREFIX("/delete?",				ProcessDelete(pars, op)) else
	IF_PREFIX("/remove?",				ProcessRemove(pars, op)) else
	IF_PREFIX("/prune?",				ProcessPrune(pars, op)) else
	IF_PREFIX("/listkeys?",				ProcessList(pars, op, false, false)) else
	IF_PREFIX("/dirtylistkeys?",		ProcessList(pars, op, false, true)) else
	IF_PREFIX("/listkeyvalues?",		ProcessList(pars, op, true, false)) else
	IF_PREFIX("/dirtylistkeyvalues?",	ProcessList(pars, op, true, true)) else
	IF_PREFIX("/count?",				ProcessCount(pars, op, false)) else
	IF_PREFIX("/dirtycount?",			ProcessCount(pars, op, true)) else
	if (ret &&
		(request.line.uri[0] == 0 ||
		(request.line.uri[0] == '/' && request.line.uri[1] == 0)))
	{
		ret = PrintHello();
		ret = false;
	}
	else
	{
		ResponseNotFound();
		ret = false;
	}
	
	if (ret && !Add(op))
	{
		ResponseFail();
		ret = false;
	}


	if (ret && op->IsWrite())
		ret = kdb->Submit();
	
	if (!ret)
	{
		delete op;
		return -1;
	}
	
	return 0;
}

#define VALIDATE_KEYLEN(bs)\
if (bs.length > KEYSPACE_KEY_SIZE) { ResponseFail(); return false; }

#define VALIDATE_VALLEN(bs)\
if (bs.length > KEYSPACE_VAL_SIZE) { ResponseFail(); return false; }

bool HttpConn::ProcessGet(const char* params, KeyspaceOp* op,
bool dirty)
{
	if (dirty)
		op->type = KeyspaceOp::DIRTY_GET;
	else
		op->type = KeyspaceOp::GET;
	
	op->key.Set((char*) params, strlen(params));
	return true;
}

bool HttpConn::ProcessList(const char* params,
KeyspaceOp* op, bool p, bool dirty)
{
	ByteString prefix, key, count, offset, direction;
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
		ResponseFail();
		return false;
	}
	op->offset = strntoint64(offset.buffer, offset.length, &nread);
	if (nread != (unsigned) offset.length)
	{
		ResponseFail();
		return false;
	}
	return true;
}

bool HttpConn::ProcessCount(const char* params,
KeyspaceOp* op, bool dirty)
{
	ByteString prefix, key, count, offset, direction;
	unsigned nread;

	if (!dirty)
		op->type = KeyspaceOp::COUNT;
	else
		op->type = KeyspaceOp::DIRTY_COUNT;
	
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
		ResponseFail();
		return false;
	}
	op->offset = strntoint64(offset.buffer, offset.length, &nread);
	if (nread != (unsigned) offset.length)
	{
		ResponseFail();
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
		ResponseFail();
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

bool HttpConn::ProcessAdminCheckpoint()
{
	// call Database::Checkpoint in an async way
	Response(200, "OK", strlen("OK"));
	database.OnCheckpointTimeout();
	return true;
}

bool HttpConn::PrintHello()
{
	ByteArray<10*KB> text;
	if (kdb->IsReplicated())
	{
		// TODO: print catching up, highest paxosID seen here
		text.length = snprintf(text.buffer, text.size,
			"Keyspace v" VERSION_STRING " r%.*s\n\n"
			"Running in replicated mode with %d nodes\n\n" \
			"I am node %d\n\n" \
			"Master is node %d%s%s\n\n" \
			"I am in replication round %" PRIu64 "\n\n" \
			"Last replication round was %d bytes, took %d msec, thruput was %d KB/sec\n",
			(int)VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER,
			RLOG->GetNumNodes(),
			RLOG->GetNodeID(),
			kdb->GetMaster(),
			kdb->IsMaster() ? " (me)" : "",
			kdb->IsMasterKnown() ? "" : " (unknown)",
			RLOG->GetPaxosID(),
			(int)RLOG->GetLastRound_Length(),
			(int)RLOG->GetLastRound_Time(),
			(int)RLOG->GetLastRound_Thruput()/1000 + 1);
	}
	else
	{
		text.length = snprintf(text.buffer, text.size,
			"Keyspace v" VERSION_STRING " r%.*s running\n\n" \
			"Running in single mode",
			(int)VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER);
	}
	
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

void HttpConn::ResponseFail()
{
	if (type == JSON)
	{
		PrintJSONStatus("error", MSG_FAIL);
		WritePending();
		closeAfterSend = true;
	}
	else
		Response(500, MSG_FAIL, sizeof(MSG_FAIL) - 1);
}

void HttpConn::ResponseNotFound()
{
	if (type == JSON)
	{
		PrintJSONStatus("error", MSG_NOT_FOUND);
		WritePending();
		closeAfterSend = true;
	}
	else
		Response(404, MSG_NOT_FOUND, sizeof(MSG_NOT_FOUND) - 1);
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
