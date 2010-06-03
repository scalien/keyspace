#include "HttpKeyspaceSession.h"
#include "Application/HTTP/HttpConn.h"
#include "Application/HTTP/HttpServer.h"
#include "Application/HTTP/HttpConsts.h"
#include "Application/HTTP/UrlParam.h"
#include "Framework/Database/Database.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Version.h"

#define MSG_FAIL			"Unable to process your request at this time"
#define MSG_NOT_FOUND		"Key not found"

#define VALIDATE_KEYLEN(bs)\
if (bs.length > KEYSPACE_KEY_SIZE) { return NULL; }

#define VALIDATE_VALLEN(bs)\
if (bs.length > KEYSPACE_VAL_SIZE) { return NULL; }

#define GET_NAMED_PARAM(params, name, var) \
if (!params.GetNamed(name, sizeof("" name) - 1, var)) { return NULL; }

#define GET_NAMED_OPT_PARAM(params, name, var) \
params.GetNamed(name, sizeof("" name) - 1, var)


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

	// here we take control of the destruction of HttpConn
	conn->SetOnClose(&onCloseConn);
}

bool HttpKeyspaceSession::MatchString(const char* s1, unsigned len1, const char* s2, unsigned len2)
{
	if (len1 != len2)
		return false;
		
	return (strncmp(s1, s2, len2) == 0);
}

char* HttpKeyspaceSession::ParseType(char* pos)
{
	type = PLAIN;
	if (strncmp(pos, "json/", 5) == 0) {
		type = JSON;
		pos += 5;
	} else if (strncmp(pos, "html/", 5) == 0) {
		type = HTML;
		pos += 5;
	}

	return pos;
}

#define STR_AND_LEN(s) s, strlen(s)

KeyspaceOp* HttpKeyspaceSession::ProcessDBCommand(const char* cmd, unsigned cmdlen, const UrlParam& params)
{
	if (MatchString(cmd, cmdlen, STR_AND_LEN("get"))) 
		return ProcessGet(params, false);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("dirtyget")))
		return ProcessGet(params, true);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("set")))
		return ProcessSet(params);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("testandset")))
		return ProcessTestAndSet(params);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("add")))
		return ProcessAdd(params);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("rename")))
		return ProcessRename(params);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("delete")))
		return ProcessDelete(params);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("remove")))
		return ProcessRemove(params);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("prune")))
		return ProcessPrune(params);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("listkeys")))
		return ProcessList(params, false, false);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("dirtylistkeys")))
		return ProcessList(params, false, true);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("listkeyvalues")))
		return ProcessList(params, true, false);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("dirtylistkeyvalues")))
		return ProcessList(params, true, true);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("count")))
		return ProcessCount(params, false);
	if (MatchString(cmd, cmdlen, STR_AND_LEN("dirtycount")))
		return ProcessCount(params, true);
	
	return NULL;
}

bool HttpKeyspaceSession::ProcessCommand(const char* cmd, unsigned cmdlen, const UrlParam& params)
{
	KeyspaceOp*	op;

	if (MatchString(cmd, cmdlen, STR_AND_LEN("")))
	{
		PrintHello();
		return true;
	}
	if (MatchString(cmd, cmdlen, STR_AND_LEN("getmaster")))
	{
		ProcessGetMaster();
		return true;
	}
	
	op = ProcessDBCommand(cmd, cmdlen, params);
	if (!op)
		return false;

	op->service = this;
	if (!Add(op))
	{
		ResponseFail();
		delete op;
		return true;
	}

	if (op->IsWrite())
		kdb->Submit();
	
	return true;
}

bool HttpKeyspaceSession::HandleRequest(const HttpRequest& request)
{
	char*		pos;
	char*		qmark;
	unsigned	cmdlen;
	UrlParam	params;

	pos = (char*) request.line.uri;
	if (*pos == '/') 
		pos++;
	
	pos = ParseType(pos);
	qmark = strchr(pos, '?');
	if (qmark)
	{
		params.Init(qmark + 1, '&');
		if (type == JSON) 
			GET_NAMED_OPT_PARAM(params, "callback", jsonCallback);
			
		cmdlen = qmark - pos;
	}
	else
		cmdlen = strlen(pos);

	return ProcessCommand(pos, cmdlen, params);
}

void HttpKeyspaceSession::PrintHello()
{
	ByteArray<10*KB> text;

	if (kdb->IsReplicated())
	{
		// TODO: print catching up, highest paxosID seen here
		text.length = snprintf(text.buffer, text.size,
			"Keyspace v" VERSION_STRING "\n\n"
			"Running in replicated mode with %d nodes\n\n"
			"I am node %d\n\n"
			"Master is node %d%s%s\n\n"
			"I am in replication round %" PRIu64 "\n\n"
			"Last replication round was %d bytes, took %d msec, thruput was %d KB/sec\n",
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
			"Keyspace v" VERSION_STRING " running\n\n" \
			"Running in single mode");
	}
	
	conn->Response(HTTP_STATUS_CODE_OK, text.buffer, text.length);
}

void HttpKeyspaceSession::ProcessGetMaster()
{
	ByteArray<10> text;
	if (kdb->IsReplicated())
		text.length = snprintf(text.buffer, text.size, "%d", kdb->GetMaster());
	else
		text.length = snprintf(text.buffer, text.size, "0");
	
	conn->Response(200, text.buffer, text.length);
}

KeyspaceOp* HttpKeyspaceSession::ProcessGet(const UrlParam& params, bool dirty)
{
	ByteString	key;
	KeyspaceOp* op;

	GET_NAMED_PARAM(params, "key", key);
	
	op = new KeyspaceOp;
	
	if (dirty)
		op->type = KeyspaceOp::DIRTY_GET;
	else
		op->type = KeyspaceOp::GET;
	
	
	op->key.Set(key);
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessList(const UrlParam& params,
bool p, bool dirty)
{
	ByteString prefix, start, count, offset, direction;
	ByteString startPostfix;
	unsigned nread;
	KeyspaceOp* op;

	GET_NAMED_OPT_PARAM(params, "prefix", prefix);
	GET_NAMED_OPT_PARAM(params, "start", start);
	GET_NAMED_OPT_PARAM(params, "count", count);
	GET_NAMED_OPT_PARAM(params, "offset", offset);
	GET_NAMED_OPT_PARAM(params, "direction", direction);
	
	VALIDATE_KEYLEN(prefix);
	VALIDATE_KEYLEN(start);

	if (start.length < prefix.length)
		return NULL;

	op = new KeyspaceOp;
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
	
	
	op->prefix.Set(prefix);
	start.Advance(prefix.length);
	op->key.Set(start);
	op->count = strntoint64(count.buffer, count.length, &nread);
	if (direction.length != 1)
		op->forward = true;
	else
		op->forward = (direction.buffer[0] == 'f');
	if (nread != (unsigned) count.length)
	{
		delete op;
		return NULL;
	}
	op->offset = strntoint64(offset.buffer, offset.length, &nread);
	if (nread != (unsigned) offset.length)
	{
		delete op;
		return NULL;
	}
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessCount(const UrlParam& params, bool dirty)
{
	ByteString prefix, start, count, offset, direction;
	ByteString startPostfix;
	unsigned nread;
	KeyspaceOp* op;
	
	GET_NAMED_OPT_PARAM(params, "prefix", prefix);
	GET_NAMED_OPT_PARAM(params, "start", start);
	GET_NAMED_OPT_PARAM(params, "count", count);
	GET_NAMED_OPT_PARAM(params, "offset", offset);
	GET_NAMED_OPT_PARAM(params, "direction", direction);
	
	VALIDATE_KEYLEN(prefix);
	VALIDATE_KEYLEN(start);
	
	if (start.length < prefix.length)
		return NULL;

	op = new KeyspaceOp;
	
	if (!dirty)
		op->type = KeyspaceOp::COUNT;
	else
		op->type = KeyspaceOp::DIRTY_COUNT;
	
	op->prefix.Set(prefix);
	start.Advance(prefix.length);
	op->key.Set(start);
	op->count = strntoint64(count.buffer, count.length, &nread);
	if (direction.length != 1)
		op->forward = true;
	else
		op->forward = (direction.buffer[0] == 'f');
	if (nread != (unsigned) count.length)
	{
		delete op;
		return NULL;
	}
	op->offset = strntoint64(offset.buffer, offset.length, &nread);
	if (nread != (unsigned) offset.length)
	{
		delete op;
		return NULL;
	}
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessSet(const UrlParam& params)
{
	ByteString key, value;
	KeyspaceOp* op;
	
	GET_NAMED_PARAM(params, "key", key);
	GET_NAMED_PARAM(params, "value", value);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(value);

	op = new KeyspaceOp;
	op->type = KeyspaceOp::SET;
	op->key.Set(key);
	op->value.Set(value);	
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessTestAndSet(const UrlParam& params)
{
	ByteString key, test, value;
	KeyspaceOp* op;
	
	GET_NAMED_PARAM(params, "key", key);
	GET_NAMED_PARAM(params, "test", test);
	GET_NAMED_PARAM(params, "value", value);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(test);
	VALIDATE_VALLEN(value);
	
	op = new KeyspaceOp;
	op->type = KeyspaceOp::TEST_AND_SET;
	
	op->key.Set(key);
	op->test.Set(test);
	op->value.Set(value);
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessAdd(const UrlParam& params)
{
	ByteString key, num;
	unsigned nread;
	KeyspaceOp* op;
	
	GET_NAMED_PARAM(params, "key", key);
	GET_NAMED_PARAM(params, "num", num);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_VALLEN(num);
	
	op = new KeyspaceOp;
	op->type = KeyspaceOp::ADD;
	
	op->key.Set(key);
	op->num	= strntoint64(num.buffer, num.length, &nread);
	if (nread != (unsigned) num.length)
	{
		delete op;
		return NULL;
	}
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessRename(const UrlParam& params)
{
	ByteString key, newKey;
	KeyspaceOp* op;
	
	GET_NAMED_PARAM(params, "from", key);
	GET_NAMED_PARAM(params, "to", newKey);
	
	VALIDATE_KEYLEN(key);
	VALIDATE_KEYLEN(newKey);
	
	op = new KeyspaceOp;
	op->type = KeyspaceOp::RENAME;
	op->key.Set(key);
	op->newKey.Set(newKey);
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessDelete(const UrlParam& params)
{
	ByteString key;
	KeyspaceOp* op;
	
	GET_NAMED_PARAM(params, "key", key);
	
	VALIDATE_KEYLEN(key);
	
	op = new KeyspaceOp;
	op->type = KeyspaceOp::DELETE;
	op->key.Set(key);
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessRemove(const UrlParam& params)
{
	ByteString key;
	KeyspaceOp* op;
	
	GET_NAMED_PARAM(params, "key", key);
	
	VALIDATE_KEYLEN(key);
	
	op = new KeyspaceOp;
	op->type = KeyspaceOp::REMOVE;
	op->key.Set(key);
	return op;
}

KeyspaceOp* HttpKeyspaceSession::ProcessPrune(const UrlParam& params)
{
	ByteString prefix;
	KeyspaceOp* op;
	
	GET_NAMED_PARAM(params, "prefix", prefix);
	
	VALIDATE_KEYLEN(prefix);
	
	op = new KeyspaceOp;
	op->type = KeyspaceOp::PRUNE;
	op->prefix.Set(prefix);
	return op;
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
				"Content-type: text/html" HTTP_CS_CRLF HTTP_CS_CRLF);
				conn->Print("<title>");
				conn->Print("Keyspace contents of: ");
				conn->Write(op->key.buffer, op->key.length, false);
				conn->Print("</title>");
				conn->Write(op->value.buffer, op->value.length, true);
			}
			else if (type == JSON)
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" HTTP_CS_CRLF HTTP_CS_CRLF);
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
			if (op->type == KeyspaceOp::GET)
				ResponseNotFound();
			else
				ResponseFail();
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
				"Content-type: text/html" HTTP_CS_CRLF HTTP_CS_CRLF);
				conn->Print("<title>");
				conn->Print("Keyspace listing of: ");
				conn->Write(op->prefix.buffer, op->prefix.length, false);
				conn->Print("</title>");
				PrintStyle();
			}
			else if (type == JSON)
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" HTTP_CS_CRLF HTTP_CS_CRLF);
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
				"Content-type: text/plain" HTTP_CS_CRLF HTTP_CS_CRLF);
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
					conn->Print("<a href='/html/get?key=");
				else
					conn->Print("<a href='/html/dirtyget?key=");
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
				"Content-type: text/html" HTTP_CS_CRLF HTTP_CS_CRLF);
				conn->Print("<title>");
				conn->Print("Keyspace listing of: ");
				conn->Write(op->prefix.buffer, op->prefix.length, false);
				conn->Print("</title>");
				PrintStyle();
			}
			else if (type == JSON)
			{
				conn->ResponseHeader(HTTP_STATUS_CODE_OK, false,
				"Content-type: text/plain" HTTP_CS_CRLF HTTP_CS_CRLF);
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
				"Content-type: text/plain" HTTP_CS_CRLF HTTP_CS_CRLF);
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
					conn->Print("<a href='/html/get?key=");
				else
					conn->Print("<a href='/html/dirtyget?key=");
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
	"Content-type: text/plain" HTTP_CS_CRLF HTTP_CS_CRLF);
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
		conn->Response(HTTP_STATUS_CODE_OK, MSG_FAIL, sizeof(MSG_FAIL) - 1);
}


void HttpKeyspaceSession::ResponseNotFound()
{
	if (type == JSON)
	{
		PrintJSONStatus("error", MSG_NOT_FOUND);
		conn->Flush(true);
	}
	else
		conn->Response(HTTP_STATUS_CODE_OK, MSG_NOT_FOUND, sizeof(MSG_NOT_FOUND) - 1);
}

void HttpKeyspaceSession::OnCloseConn()
{
	if (conn->GetState() == HttpConn::DISCONNECTED && numpending == 0)
		delete this;
}
