#include "HttpApiHandler.h"
#include "Application/HTTP/HttpConsts.h"
#include "Application/HTTP/JSONSession.h"
#include "Application/HTTP/UrlParam.h"
#include "System/Config.h"
#include "System/Common.h"
#include "Framework/Database/Database.h"
#include "Version.h"

typedef bool (HttpApiHandler::*HandlerFunc)(JSONSession* session, const UrlParam& param);
struct HandlerMap
{
public:
	const char* name;
	size_t		namelen;
	HandlerFunc	func;
};

#define HANDLER_NAME(name) (name), sizeof(name)-1

static HandlerMap handlerMap[] = {
	{HANDLER_NAME("config"), &HttpApiHandler::GetConfigVar},
	{HANDLER_NAME("checkpoint"), &HttpApiHandler::Checkpoint},
	{HANDLER_NAME("trace"), &HttpApiHandler::LogTrace},
	{HANDLER_NAME("status"), &HttpApiHandler::Status},
	{HANDLER_NAME("segfault"), &HttpApiHandler::Segfault},
	{HANDLER_NAME("exit"), &HttpApiHandler::Exit},
	{HANDLER_NAME("restart"), &HttpApiHandler::Restart},
};

HttpApiHandler::HttpApiHandler(KeyspaceDB* kdb_)
{
	kdb = kdb_;
}

bool HttpApiHandler::HandleRequest(HttpConn* conn, const HttpRequest& request)
{
	const char		prefix[] = "/api";
	const char*		p;
	size_t			i;
	ByteString		jsonCallback;
	JSONSession*	jsonSession;
	UrlParam		param;
	bool			ret;
	
	if (strncmp(request.line.uri, prefix, sizeof(prefix) - 1))
		return false;

	p = request.line.uri + sizeof(prefix) - 1;
	if (*p == ',')
	{
		p++;
		UrlParam_Parse(p, '/', 1, &jsonCallback);
		if (jsonCallback.length)
			p += jsonCallback.length;
	}

	p++;
	
	jsonSession = new JSONSession;
	jsonSession->Init(conn, jsonCallback);
	ret = false;
	for (i = 0; i < SIZE(handlerMap); i++)
	{
		if (strncmp(p, handlerMap[i].name, handlerMap[i].namelen) == 0)
		{
			p += handlerMap[i].namelen;
			if (*p == '?')
			{
				p++;
				param.Init(p, '&');
			}
			
			ret = (this->*handlerMap[i].func)(jsonSession, param);
			break;
		}
	}
	
	delete jsonSession;
	return ret;
}

bool HttpApiHandler::Checkpoint(JSONSession* jsonSession, const UrlParam&)
{
	// call Database::Checkpoint in an async way
	database.OnCheckpointTimeout();
	jsonSession->PrintStatus("OK");
	
	return true;
}

bool HttpApiHandler::GetConfigVar(JSONSession* jsonSession, const UrlParam& param)
{
	DynArray<128>	text;
	const char		nameArg[] = "name";
	const char*		p;
	int				narg;
	
	narg = param.GetParamIndex(nameArg);
	if (narg >= 0)
	{
		p = param.GetParam(narg);
		p += sizeof(nameArg);
		text.Writef("%s: %s", p, Config::GetValue(p, ""));
		jsonSession->PrintStatus(text.buffer);
	}
	else
	{
		jsonSession->PrintStatus("ERROR");
	}
	
	return true;
}

bool HttpApiHandler::LogTrace(JSONSession* jsonSession, const UrlParam& /*param*/)
{
	DynArray<128>	text;

//	if (!param.GetParam(0))
//	{
//		text.Writef("%s", Log_GetTrace() ? "true" : "false");
//	}
//	else
//	{
//		text.Writef("ERROR");
//		if (*ar == '/')
//		{
//			args++;
//			if (strcmp(args, "true") == 0)
//			{
//				text.Writef("OK");
//				Log_SetTrace(true);
//			}
//			else if (strcmp(args, "false") == 0)
//			{
//				text.Writef("OK");
//				Log_SetTrace(false);
//			}
//		}
//	}
	text.Append("", 1);
	
	jsonSession->PrintStatus(text.buffer);

	return true;
}

bool HttpApiHandler::Status(JSONSession* jsonSession, const UrlParam&)
{
	ByteArray<128> text;
	
	text.length = snprintf(text.buffer, text.size,
		"Keyspace v" VERSION_STRING " (%s) running,Master: %d%s%s",
		PLATFORM_STRING,
		kdb->GetMaster(),
		kdb->IsMaster() ? " (me)" : "",
		kdb->IsMasterKnown() ? "" : " (unknown)");

	jsonSession->PrintStatus(text.buffer);

	return true;
}

bool HttpApiHandler::Segfault(JSONSession*, const UrlParam&)
{
	((char*)NULL)[0] = 0;
	return false;
}

bool HttpApiHandler::Exit(JSONSession*, const UrlParam& param)
{
	int			exitCode = 0;
	ByteString	code;
	unsigned	nread;
	
	if (param.GetParam(0))
		exitCode = (int) strntoint64(param.GetParam(0), param.GetParamLen(0), &nread);
	
	STOP_FAIL("Exit initiated from web console", exitCode);

	return false;
}

bool HttpApiHandler::Restart(JSONSession*, const UrlParam&)
{
	RESTART("Restart initiated from web console");
	return false;
}

