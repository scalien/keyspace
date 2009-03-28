#include "CatchupServer.h"
#include "CatchupConn.h"

bool CatchupServer::Init(int port)
{
	table = database.GetTable("keyspace");
	
	return TCPServer::Init(port);
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
		conn->Init(true);
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	IOProcessor::Get()->Add(&tcpread);
}
