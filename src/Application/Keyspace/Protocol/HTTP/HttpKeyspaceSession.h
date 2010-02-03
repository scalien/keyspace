#ifndef HTTP_KEYSPACE_SESSION_H
#define HTTP_KEYSPACE_SESSION_H

#include "System/Events/Callable.h"
#include "Application/Keyspace/Database/KeyspaceService.h"

class HttpConn;
class HttpRequest;
class UrlParam;

class HttpKeyspaceSession : public KeyspaceService
{
public:
	HttpKeyspaceSession(KeyspaceDB* kdb_);
	~HttpKeyspaceSession();

	void			Init(HttpConn* conn_);

	// KeyspaceService interface
	virtual void	OnComplete(KeyspaceOp* op, bool final);
	virtual bool	IsAborted();

	bool			HandleRequest(const HttpRequest& request);

	bool			ProcessGetMaster();
	bool			ProcessGet(const UrlParam& params,
					KeyspaceOp* op, bool dirty);
	bool			ProcessList(const UrlParam& params,
					KeyspaceOp* op, bool p, bool dirty);
	bool			ProcessCount(const UrlParam& params,
					KeyspaceOp* op, bool dirty);
	bool			ProcessSet(const UrlParam& params, KeyspaceOp* op);
	bool			ProcessTestAndSet(const UrlParam& params, KeyspaceOp* op);
	bool			ProcessAdd(const UrlParam& params, KeyspaceOp* op);
	bool			ProcessRename(const UrlParam& params, KeyspaceOp* op);
	bool			ProcessDelete(const UrlParam& params, KeyspaceOp* op);
	bool			ProcessRemove(const UrlParam& params, KeyspaceOp* op);
	bool			ProcessPrune(const UrlParam& params, KeyspaceOp* op);

	bool			PrintHello();
	void			PrintStyle();
	void			PrintJSONString(const char* s, unsigned len);
	void			PrintJSONStatus(const char* status, const char* type = NULL);
	void			ResponseFail();
	void			ResponseNotFound();
	
	void			OnCloseConn();
	
private:
	typedef MFunc<HttpKeyspaceSession> Func;
	enum Type
	{
		PLAIN,
		HTML,
		JSON
	};
	
	Func			onCloseConn;
	HttpConn*		conn;
	bool			headerSent;
	Type			type;
	ByteString		jsonCallback;
	bool			rowp;
};

#endif
