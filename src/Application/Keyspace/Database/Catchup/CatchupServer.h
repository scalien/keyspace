#ifndef CATCHUPSERVER_H
#define CATCHUPSERVER_H

#include "Framework/Transport/TCPServer.h"
#include "Framework/Database/Table.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"

class CatchupServer : public TCPServer
{
friend class CatchupConn;

public:
	bool				Init(IOProcessor* ioproc_, int port, Table* table_, ReplicatedLog* replicatedLog_);
	
	void				OnConnect();

private:
	Table*				table;
	ReplicatedLog*		replicatedLog;
};

#endif
