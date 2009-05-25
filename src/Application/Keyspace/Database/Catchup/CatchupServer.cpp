#include "CatchupServer.h"
#include "CatchupConn.h"

void CatchupServer::Init(int port_)
{
	if (!TCPServerT<CatchupServer, CatchupConn, PAXOS_BUF_SIZE>::Init(port_, CONN_BACKLOG))
		STOP_FAIL("Cannot initialize CatchupServer", 1);
}

void CatchupServer::InitConn(CatchupConn* conn)
{
	if (numActive > 1)
	{
		conn->Close();
		DeleteConn(conn);
		return;
	}

	conn->Init();
}
