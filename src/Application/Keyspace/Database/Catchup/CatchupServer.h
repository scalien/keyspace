#ifndef CATCHUPSERVER_H
#define CATCHUPSERVER_H

#include "Framework/Transport/TCPServer.h"
#include "Framework/Database/Table.h"
#include "Framework/ReplicatedLog/ReplicatedLog.h"
#include "Framework/Paxos/PaxosConsts.h"
#include "CatchupConn.h"

#define CONN_BACKLOG	2

class CatchupServer : public TCPServerT<CatchupServer, CatchupConn, PAXOS_BUF_SIZE>
{
public:
	void			Init(int port);
	void			InitConn(CatchupConn* conn);
};

#endif
