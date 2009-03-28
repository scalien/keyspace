#ifndef CATCHUPSERVER_H
#define CATCHUPSERVER_H

#include "Framework/Transport/TCPServer.h"
#include "Framework/Database/Table.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"

class CatchupServer : public TCPServer
{
friend class CatchupConn;

public:
	bool				Init(int port);
	
	void				OnConnect();

private:
	Table*				table;
	ReplicatedLog*		replicatedLog;
};

#endif
