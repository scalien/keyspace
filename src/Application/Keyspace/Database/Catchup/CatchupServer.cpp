#include "CatchupServer.h"
#include "CatchupConn.h"

bool CatchupServer::Init(int port)
{
	Log_Trace();
	
	table = database.GetTable("keyspace");
	
	return TCPServer::Init(port);
}

void CatchupServer::OnConnect()
{
	Log_Trace();
	
	CatchupConn* conn = new CatchupConn(this);
	
	if (listener.Accept(&(conn->GetSocket())))
	{
		Endpoint endpoint;
		conn->GetSocket().GetEndpoint(endpoint);
		
		Log_Message("%s connected", endpoint.ToString());
		
		conn->GetSocket().SetNonblocking();
		conn->Init();
	}
	else
	{
		Log_Message("Accept() failed");
		delete conn;
	}
	
	IOProcessor::Add(&tcpread);
}
