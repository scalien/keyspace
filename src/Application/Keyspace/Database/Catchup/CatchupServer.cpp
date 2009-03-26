#include "CatchupServer.h"
#include "CatchupConn.h"

bool CatchupServer::Init(IOProcessor* ioproc_, int port, Table* table_, ReplicatedLog* replicatedLog_)
{
	table = table_;
	replicatedLog = replicatedLog_;
	
	return TCPServer::Init(ioproc_, port);
}

void CatchupServer::OnConnect()
{
	CatchupConn* conn = new CatchupConn(this);
	
	if (listener.Accept(&(conn->GetSocket())))
	{
		Endpoint endpoint;
		conn->GetSocket().GetEndpoint(endpoint);
		
		Log_Message("%s connected", endpoint.ToString());
		
		conn->GetSocket().SetNonblocking();
		conn->Init(ioproc, true);
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	ioproc->Add(&tcpread);
}
