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

	void			OnCloseConn();
	
private:
	typedef MFunc<HttpKeyspaceSession> Func;
	enum Type
	{
		PLAIN,
		HTML,
		JSON
	};

	bool			GetNamedParam(const UrlParam& params, 
					const char* name, int namelen,
					ByteString& arg);
	
	void			PrintHello();
	void			ProcessGetMaster();
	bool			ProcessCommand(const char* cmd, unsigned cmdlen, 
					const UrlParam& params);
	KeyspaceOp*		ProcessDBCommand(const char* cmd, unsigned cmdlen, 
					const UrlParam& params);
	KeyspaceOp*		ProcessGet(const UrlParam& params, bool dirty);
	KeyspaceOp*		ProcessList(const UrlParam& params, bool pair, bool dirty);
	KeyspaceOp*		ProcessCount(const UrlParam& params, bool dirty);
	KeyspaceOp*		ProcessSet(const UrlParam& params);
	KeyspaceOp*		ProcessTestAndSet(const UrlParam& params);
	KeyspaceOp*		ProcessAdd(const UrlParam& params);
	KeyspaceOp*		ProcessRename(const UrlParam& params);
	KeyspaceOp*		ProcessDelete(const UrlParam& params);
	KeyspaceOp* 	ProcessRemove(const UrlParam& params);
	KeyspaceOp*		ProcessPrune(const UrlParam& params);
	KeyspaceOp*		ProcessSetExpiry(const UrlParam& params);
	KeyspaceOp*		ProcessRemoveExpiry(const UrlParam& params);
	
	void			PrintStyle();
	void			PrintJSONString(const char* s, unsigned len);
	void			PrintJSONStatus(const char* status, const char* type = NULL);
	void			ResponseFail();
	void			ResponseNotFound();
	
	char*			ParseType(char* pos);
	bool			MatchString(const char* s1, unsigned len1, 
					const char* s2, unsigned len2);
	
	Func			onCloseConn;
	HttpConn*		conn;
	bool			headerSent;
	Type			type;
	ByteString		jsonCallback;
	bool			rowp;
};

#endif
