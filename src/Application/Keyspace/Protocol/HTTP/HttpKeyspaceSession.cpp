#include "HttpKeyspaceSession.h"
#include "Application/HTTP/HttpConn.h"
#include "Application/HTTP/HttpServer.h"
#include "Application/HTTP/HttpConsts.h"
#include "Application/HTTP/UrlParam.h"
#include "Framework/Database/Database.h"
#include "Version.h"

#define CS_CR				"\015"
#define CS_LF				"\012"
#define CS_CRLF				CS_CR CS_LF
#define MSG_FAIL			"Unable to process your request at this time"
#define MSG_NOT_FOUND		"Not found"
#define PARAMSEP			','

HttpKeyspaceSession::HttpKeyspaceSession(KeyspaceDB* kdb_) :
onCloseConn(this, &HttpKeyspaceSession::OnCloseConn)
{
	KeyspaceService::Init(kdb_);
}

HttpKeyspaceSession::~HttpKeyspaceSession()
{
	if (conn)
		conn->GetServer()->DeleteConn(conn);
}

void HttpKeyspaceSession::Init(HttpConn* conn_)
{
	conn = conn_;

	headerSent = false;
	type = PLAIN;
	rowp = false;
	jsonCallback.Init();
}


bool HttpKeyspaceSession::HandleRequest(const HttpRequest& request)
{
#define IF_PREFIX(prefix, func) \
if (ret && strncmp(pars, prefix, strlen(prefix)) == 0) \
{ \
	pars += strlen(prefix); \
	params.Init(pars, PARAMSEP); \
	ret = func; \
}

	UrlParam	params;
	KeyspaceOp* op;
	op = new KeyspaceOp;
	op->service = this;
	
	// note that these variables are used in IF_PREFIX macro
	char* pars = (char*) request.line.uri;
	bool ret = true;

	// first see if the url contains the response type (json or html)
	type = PLAIN;
	IF_PREFIX("/json", (type = JSON, true)) else
	IF_PREFIX("/html", (type = HTML, true));
	
	// special case for JSONP callback
	if (type == JSON && *pars == PARAMSEP)
	{
		pars++;
		UrlParam_Parse(pars, '/', 1, &jsonCallback);
		if (jsonCallback.length == 0)
			ret = false;
		else
			pars += jsonCallback.length;
	}
	
	// parse the command part of the request uri
	IF_PREFIX("/getmaster",				ProcessGetMaster()) else
	IF_PREFIX("/get?",					ProcessGet(params, op, false)) else
	IF_PREFIX("/dirtyget?",				ProcessGet(params, op, true)) else
	IF_PREFIX("/set?",					ProcessSet(params, op)) else
	IF_PREFIX("/testandset?",			ProcessTestAndSet(params, op)) else
	IF_PREFIX("/add?",					ProcessAdd(params, op)) else
	IF_PREFIX("/rename?",				ProcessRename(params, op)) else
	IF_PREFIX("/delete?",				ProcessDelete(params, op)) else
	IF_PREFIX("/remove?",				ProcessRemove(params, op)) else
	IF_PREFIX("/prune?",				ProcessPrune(params, op)) else
	IF_PREFIX("/listkeys?",				ProcessList(params, op, false, false)) else
	IF_PREFIX("/dirtylistkeys?",		ProcessList(params, op, false, true)) else
	IF_PREFIX("/listkeyvalues?",		ProcessList(params, op, true, false)) else
	IF_PREFIX("/dirtylistkeyvalues?",	ProcessList(params, op, true, true)) else
	IF_PREFIX("/count?",				ProcessCount(params, op, false)) else
	IF_PREFIX("/dirtycount?",			ProcessCount(params, op, true)) else
	if (ret &&
		(request.line.uri[0] == 0 ||
		(request.line.uri[0] == '/' && request.line.uri[1] == 0)))
	{
		ret = PrintHello();
		ret = true;
	}
	else
		return false; // did not found prefix

	// here we take control of the destruction of HttpConn
	conn->SetOnClose(&onCloseConn);
	
	if (ret && !Add(op))
	{
		ResponseFail();
		ret = true;
	}

	if (ret && op->IsWrite())
		ret = kdb->Submit();
	
	if (!ret)
	{
		delete op;
		return true;
	}
	
	
	return true;
}

#define VALIDATE_KEYLEN(bs)\
if (bs.length > KEYSPACE_KEY_SIZE) { ResponseFail(); return false; }

#define VALIDATE_VALLEN(bs)\
if (bs.length > KEYSPACE_VAL_SIZE) { ResponseFail(); return false; }

bool HttpKeyspaceSession::ProcessGetMaster()
{
	ByteArray<10> text;
	if (kdb->IsReplicated())
		text.length = snprintf(text.buffer, text.size, "%d", kdb->GetMaster());
	else
		text.length = snprintf(text.buffer, text.size, "0");
	
	conn->Response(200, text.buffer, text.length);
	return false;
}

bool HttpKeyspaceSession::ProcessGet(const UrlParam& params, KeyspaceOp* op,
bool dirty)
{
	if (dirty)
		op->type = KeyspaceOp::DIRTY_GET;
	else
		op->type = KeyspaceOp::GET;
	
	op->key.Set(params.GetParam(0), params.GetParamLen(0));
	return true;
}

bool HttpKeyspaceSession::ProcessList(const UrlParam& params,
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
	
	params.Get(5, &prefix, &key, &count, &offset, &direction);
	
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

bool HttpKeyspaceSession::ProcessCount(const UrlParam& params,
KeyspaceOp* op, bool dirty)
{
	ByteString prefix, key, count, offset, direction;
	unsigned nread;

	if (!dirty)
		op->type = KeyspaceOp::COUNT;
	else
		op->type = KeyspaceOp::DIRTY_COUNT;
	
	params.Get(5, &prefix, &key, &count, &offset, &direction);
	
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

bool HttpKeyspaceSession::ProcessSet(const UrlParam& params, KeyspaceOp* op)
{
	ByteString key, value;

	params.Get(2, &key, &value);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(value);
	
	op->type = KeyspaceOp::SET;
	op->key.Set(key);
	op->value.Set(value);	
	return true;
}

bool HttpKeyspaceSession::ProcessTestAndSet(const UrlParam& params, KeyspaceOp* op)
{
	ByteString key, test, value;

	params.Get(3, &key, &test, &value);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(test);
	VALIDATE_VALLEN(value);
	
	op->type = KeyspaceOp::TEST_AND_SET;
	
	op->key.Set(key);
	op->test.Set(test);
	op->value.Set(value);
	return true;
}

bool HttpKeyspaceSession::ProcessAdd(const UrlParam& params, KeyspaceOp* op)
{
	ByteString key, value;
	unsigned nread;

	params.Get(2, &key, &value);
	
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

bool HttpKeyspaceSession::ProcessRename(const UrlParam& params, KeyspaceOp* op)
{
	ByteString key, newKey;

	params.Get(2, &key, &newKey);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_KEYLEN(newKey);
	
	op->type = KeyspaceOp::RENAME;
	op->key.Set(key);
	op->newKey.Set(newKey);
	return true;
}

bool HttpKeyspaceSession::ProcessDelete(const UrlParam& params, KeyspaceOp* op)
{
	ByteString key;

	params.Get(1, &key);
	
	VALIDATE_KEYLEN(key);
	
	op->type = KeyspaceOp::DELETE;
	op->key.Set(key);
	return true;
}

bool HttpKeyspaceSession::ProcessRemove(const UrlParam& params, KeyspaceOp* op)
{
	ByteString key;

	params.Get(1, &key);
	
	VALIDATE_KEYLEN(key);
	
	op->type = KeyspaceOp::REMOVE;
	op->key.Set(key);
	return true;
}

bool HttpKeyspaceSession::ProcessPrune(const UrlParam& params, KeyspaceOp* op)
{
	ByteString prefix;

	params.Get(1, &prefix);
	
	VALIDATE_KEYLEN(prefix);
	
	op->type = KeyspaceOp::PRUNE;
	op->prefix.Set(prefix);
	return true;
}

bool HttpKeyspaceSession::PrintHello()
{
	ByteArray<128> text;
	text.length = snprintf(text.buffer, text.size,
		"Keyspace v" VERSION_STRING " r%.*s running\n\nMaster: %d%s%s",
		(int)VERSION_REVISION_LENGTH, VERSION_REVISION_NUMBER,
		kdb->GetMaster(),
		kdb->IsMaster() ? " (me)" : "",
		kdb->IsMasterKnown() ? "" : " (unknown)");

	conn->Response(HTTP_STATUS_CODE_OK, text.buffer, text.length);
	return true;
}

void HttpKeyspaceSession::OnComplete(KeyspaceOp* op, bool final)
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
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				conn->Print("<title>");
				conn->Print("Keyspace contents of: ");
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print("</title>");
				conn->Write(op->value.buffer, op->value.length, true);
			}
			else if (type == JSON)
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
				if (jsonCallback.length)
				{
					conn->Write(jsonCallback.buffer, jsonCallback.length, false);
					conn->Print("(");
				}
				conn->Print("{");
				PrintJSONString(op->key.buffer, op->key.length);
				conn->Print(":");
				PrintJSONString(op->value.buffer, op->value.length);
				conn->Print("}");
				if (jsonCallback.length)
					conn->Print(")");
			}
			else
				conn->Response(HTTP_STATUS_CODE_OK, op->value.buffer, op->value.length);
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
				conn->Response(HTTP_STATUS_CODE_OK, "OK", strlen("OK"));
		else
			if (type == JSON)
				PrintJSONStatus("failed");
			else
				conn->Response(HTTP_STATUS_CODE_OK, "Failed", strlen("Failed"));
	}
	else if (op->type == KeyspaceOp::DELETE ||
			 op->type == KeyspaceOp::PRUNE ||
			 op->type == KeyspaceOp::RENAME)
	{
		if (op->status)
			if (type == JSON)
				PrintJSONStatus("ok");
			else
				conn->Response(HTTP_STATUS_CODE_OK, "OK", strlen("OK"));
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
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				conn->Print("<title>");
				conn->Print("Keyspace listing of: ");
				conn->Write(op->prefix.buffer, op->prefix.length, false);
				conn->Print("</title>");
				PrintStyle();
			}
			else if (type == JSON)
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
				if (jsonCallback.length)
				{
					conn->Write(jsonCallback.buffer, jsonCallback.length, false);
					conn->Print("(");
				}
				conn->Print("{");
				PrintJSONString(op->prefix.buffer, op->prefix.length);
				conn->Print(":[");
			}
			else
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
			}
			headerSent = true;
		}
		if (op->key.length > 0)
		{
			if (type == HTML)
			{
				if (rowp)
					conn->Print("<div class='even'>");
				else
					conn->Print("<div class='odd'>");
				if (op->type == KeyspaceOp::LIST)
					conn->Print("<a href='/html/get?");
				else
					conn->Print("<a href='/html/dirtyget?");
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print("'>");
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print("</a></div>");
				rowp = !rowp;
			}
			else if (type == JSON)
			{
				// with JSON rowp indicates that it is the first item or not
				if (rowp)
					conn->Print(",");
				else
					rowp = true;

				PrintJSONString(op->key.buffer, op->key.length);
			}
			else
			{
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print("\n");
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
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/html" CS_CRLF CS_CRLF);
				conn->Print("<title>");
				conn->Print("Keyspace listing of: ");
				conn->Write(op->prefix.buffer, op->prefix.length, false);
				conn->Print("</title>");
				PrintStyle();
			}
			else if (type == JSON)
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
				if (jsonCallback.length)
				{
					conn->Write(jsonCallback.buffer, jsonCallback.length, false);
					conn->Print("(");
				}
				conn->Print("{");
				PrintJSONString(op->prefix.buffer, op->prefix.length);
				conn->Print(":{");
			}
			else
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" CS_CRLF CS_CRLF);
			}
			headerSent = true;
		}
		if (op->key.length > 0)
		{
			if (type == HTML)
			{
				if (rowp)
					conn->Print("<div class='even'>");
				else
					conn->Print("<div class='odd'>");
				if (op->type == KeyspaceOp::LIST)
					conn->Print("<a href='/html/get?");
				else
					conn->Print("<a href='/html/dirtyget?");
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print("'>");
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print("</a>");
				conn->Print(" => ");
				conn->Print("<span class='value'>");
				conn->Write(op->value.buffer, op->value.length, false);
				conn->Print("</span>");
				conn->Print("</div>");
				rowp = !rowp;
			}
			else if (type == JSON)
			{
				// with JSON rowp indicates that it is the first item or not
				if (rowp)
					conn->Print(",");
				else
					rowp = true;

				PrintJSONString(op->key.buffer, op->key.length);
				conn->Print(":");
				PrintJSONString(op->value.buffer, op->value.length);
			}
			else
			{
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print(" => ");
				conn->Write(op->value.buffer, op->value.length, false);
				conn->Print("\n");
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
				conn->Print("}");
			}
			else
				conn->Print("]");
			conn->Print("}");
			if (jsonCallback.length)
				conn->Print(")");
		}
		
		conn->Flush(true); // flush data to TCP socket
		numpending--;
		delete op;
	}

	if (conn->GetState() == HttpConn::DISCONNECTED && numpending == 0)
		delete this;
}


bool HttpKeyspaceSession::IsAborted()
{
	if (conn->GetState() == HttpConn::DISCONNECTED)
		return true;
	return false;
}

void HttpKeyspaceSession::PrintJSONString(const char *s, unsigned len)
{
	conn->Write("\"", 1, false);
	for (unsigned i = 0; i < len; i++)
	{
		if (s[i] == '"')
			conn->Write("\\", 1, false);
		conn->Write(s + i, 1, false);
	}
	conn->Write("\"", 1, false);
}


void HttpKeyspaceSession::PrintJSONStatus(const char* status, const char* type_)
{
	conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
	"Content-type: text/plain" CS_CRLF CS_CRLF);
	if (jsonCallback.length)
	{
		conn->Write(jsonCallback.buffer, jsonCallback.length, false);
		conn->Print("(");
	}
	conn->Print("{\"status\":\"");
	conn->Print(status);
	if (type_)
	{
		conn->Print("\",\"type\":\"");
		conn->Print(type_);
	}
	conn->Print("\"}");

	if (jsonCallback.length)
		conn->Print(")");				
}

void HttpKeyspaceSession::PrintStyle()
{
	conn->Print("<style>");
	conn->Print("div { padding: 4px; ");
	conn->Print("      font-size: 12px; font-family: courier; }");
	conn->Print("div.even { background-color: white; }");
	conn->Print("div.odd { background-color: #F0F0F0; }");
	conn->Print("a:link { text-decoration:none; }");
	conn->Print("a:visited { text-decoration:none; }");
	conn->Print("a:hover { text-decoration: underline; }");
	conn->Print("span.value { color: #006600; }");
	conn->Print("</style>");
}

void HttpKeyspaceSession::ResponseFail()
{
	if (type == JSON)
	{
		PrintJSONStatus("error", MSG_FAIL);
		conn->Flush();
	}
	else
		conn->Response(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR, MSG_FAIL, sizeof(MSG_FAIL) - 1);
}


void HttpKeyspaceSession::ResponseNotFound()
{
	if (type == JSON)
	{
		PrintJSONStatus("error", MSG_NOT_FOUND);
		conn->Flush(true);
	}
	else
		conn->Response(HTTP_STATUS_CODE_NOT_FOUND, MSG_NOT_FOUND, sizeof(MSG_NOT_FOUND) - 1);
}

void HttpKeyspaceSession::OnCloseConn()
{
	if (conn->GetState() == HttpConn::DISCONNECTED && numpending == 0)
		delete this;	
}
