#include "CatchupServer.h"
#include "CatchupWriter.h"

void CatchupServer::Init(int port_)
{
	if (!TCPServerT<CatchupServer, CatchupWriter>::Init(port_, CONN_BACKLOG))
		STOP_FAIL("Cannot initialize CatchupServer", 1);
}

void CatchupServer::Shutdown()
{
	Close();
}

void CatchupServer::InitConn(CatchupWriter* conn)
{
	if (numActive > 1)
	{
		Log_Trace("@@@ have an active catchup connection, closing this @@@");
		conn->Close();
		DeleteConn(conn);
		return;
	}

	conn->Init(this);
}
