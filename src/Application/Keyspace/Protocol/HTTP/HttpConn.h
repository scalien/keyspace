#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "System/Containers/LinkedList.h"
#include "Framework/Transport/TCPConn.h"
#include "HttpRequest.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"
#include "Application/Keyspace/Database/KeyspaceService.h"

class HttpServer;

class HttpConn : public TCPConn<>, public KeyspaceService
{
public:
	HttpConn();
	
	void			Init(KeyspaceDB* kdb_, HttpServer* server_);

	// KeyspaceService interface
	virtual void	OnComplete(KeyspaceOp* op, bool final);
	virtual bool	IsAborted();

private:
	enum Type
	{
		PLAIN,
		HTML,
		JSON
	};
	
	HttpServer*		server;
	HttpRequest		request;
	Endpoint		endpoint;
	bool			headerSent;
	bool			closeAfterSend;
	Type			type;
	ByteString		jsonCallback;
	bool			rowp;

	// TCPConn interface
	virtual void	OnRead();
	virtual void	OnClose();
	virtual void	OnWrite();
	
	void			Print(const char* s);
	void			PrintJSONString(const char* s, unsigned len);
	void			PrintJSONStatus(const char* status, const char* type = NULL);
	int				Parse(char* buf, int len);
	int				ProcessGetRequest();
	const char*		Status(int code);
	void			Response(int code, const char* buf,
					int len, bool close = true, const char* header = NULL);
	void			ResponseHeader(int code, bool close = true,
					const char* header = NULL);
	void			ResponseFail();
	void			ResponseNotFound();
	
	bool			ProcessGetMaster();
	bool			ProcessGet(const char* params,
					KeyspaceOp* op, bool dirty);
	bool			ProcessList(const char* params,
					KeyspaceOp* op, bool p, bool dirty);
	bool			ProcessCount(const char* params,
					KeyspaceOp* op, bool dirty);
	bool			ProcessSet(const char* params, KeyspaceOp* op);
	bool			ProcessTestAndSet(const char* params, KeyspaceOp* op);
	bool			ProcessAdd(const char* params, KeyspaceOp* op);
	bool			ProcessRename(const char* params, KeyspaceOp* op);
	bool			ProcessDelete(const char* params, KeyspaceOp* op);
	bool			ProcessRemove(const char* params, KeyspaceOp* op);
	bool			ProcessPrune(const char* params, KeyspaceOp* op);
	bool			ProcessAdminCheckpoint();
	bool			PrintHello();
	void			PrintStyle();
};

#endif
