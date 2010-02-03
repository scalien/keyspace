#include "HttpKeyspaceHandler.h"
#include "HttpKeyspaceSession.h"

HttpKeyspaceHandler::HttpKeyspaceHandler(KeyspaceDB* kdb_)
{
	kdb = kdb_;
}

bool HttpKeyspaceHandler::HandleRequest(HttpConn* conn, const HttpRequest& request)
{	
	HttpKeyspaceSession* session;
	session = new HttpKeyspaceSession(kdb);
	session->Init(conn);
	return session->HandleRequest(request);
}

