#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "TCPServer.h"
#include "HttpConn.h"

class KeyspaceDB;

class HttpServer : public TCPServer
{
public:
	void			Init(KeyspaceDB* kdb);
private:
	KeyspaceDB*		kdb;
	virtual void	OnConnect();
};

#endif
