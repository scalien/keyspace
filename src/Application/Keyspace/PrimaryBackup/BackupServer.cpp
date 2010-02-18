#include "BackupServer.h"
#include "BackupConn.h"
#include "Application/Keyspace/Database/KeyspaceService.h"

#define CONN_BACKLOG	10

void BackupServer::InitServer(KeyspaceDB* kdb_, int port_)
{
	if (!TCPServerT<BackupServer, BackupConn>::Init(port_, CONN_BACKLOG))
		STOP_FAIL("Cannot initialize BackupServer", 1);
	kdb = kdb_;
	cmdID = 0;
}

bool BackupServer::Init()
{
	return kdb->Init();
}

void BackupServer::Shutdown()
{
	kdb->Shutdown();
}

bool BackupServer::Add(KeyspaceOp* op)
{
	bool			ret;
	BackupConn**	it;
	KeyspaceMsg		msg;

	ret = kdb->Add(op);
	if (ret && op->IsWrite())
	{
		msg.FromKeyspaceOp(op);
		
		for (it = activeConns.Head(); it; it = activeConns.Next(it))
			(*it)->WriteKeyspaceMsg(msg);
	}
	
	return ret;
}

bool BackupServer::Submit()
{
	// TODO
	bool	ret;

	ret = kdb->Submit();
	if (ret)
	{
		cmdID++;
	}
	
	return ret;
}

unsigned BackupServer::GetNodeID()
{
	return kdb->GetNodeID();
}

bool BackupServer::IsMasterKnown()
{
	return kdb->IsMasterKnown();
}

int BackupServer::GetMaster()
{
	return kdb->GetMaster();
}

bool BackupServer::IsMaster()
{
	return kdb->IsMaster();
}

bool BackupServer::IsReplicated()
{
	if (activeConns.Length() > 0)
		return true;
	else
		return false;
}

void BackupServer::SetProtocolServer(ProtocolServer* pserver)
{
	kdb->SetProtocolServer(pserver);
}

uint64_t BackupServer::GetLastID()
{
	return cmdID;
}

uint64_t BackupServer::GetLastKnownID()
{
	return cmdID;
}

void BackupServer::RegisterBackupConn(BackupConn* conn)
{
	activeConns.Append(conn);
}

