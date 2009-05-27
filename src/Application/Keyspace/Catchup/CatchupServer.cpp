#include "CatchupServer.h"
#include "CatchupWriter.h"

void CatchupServer::Init(int port_)
{
	if (!TCPServerT<CatchupServer, CatchupWriter, PAXOS_BUF_SIZE>::Init(port_, CONN_BACKLOG))
		STOP_FAIL("Cannot initialize CatchupServer", 1);
}

void CatchupServer::InitConn(CatchupWriter* conn)
{
	if (numActive > 1)
	{
		conn->Close();
		DeleteConn(conn);
		return;
	}

	conn->Init();
}
