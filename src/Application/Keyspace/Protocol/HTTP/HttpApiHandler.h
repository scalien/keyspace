#ifndef HTTP_API_HANDLER_H
#define HTTP_API_HANDLER_H

#include "HttpServer.h"

class JSONSession;
class UrlParam;

class HttpApiHandler : public HttpHandler
{
public:
	HttpApiHandler(KeyspaceDB* kdb);

	virtual bool	HandleRequest(HttpConn* conn, const HttpRequest& request);

	bool			Checkpoint(JSONSession* jsonSession, const UrlParam& param);
	bool			GetConfigVar(JSONSession* jsonSession, const UrlParam& param);
	bool			LogTrace(JSONSession* jsonSession, const UrlParam& param);
	bool			Status(JSONSession* jsonSession, const UrlParam& param);
	bool			Segfault(JSONSession* jsonSession, const UrlParam& param);
	bool			Exit(JSONSession* jsonSession, const UrlParam& param);
	bool			Restart(JSONSession* jsonSession, const UrlParam& param);

private:	
	KeyspaceDB*		kdb;
};

#endif
