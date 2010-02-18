#ifndef BACKUP_SERVER_H
#define BACKUP_SERVER_H

#include "BackupConn.h"
#include "Framework/Transport/TCPServer.h"
#include "Application/Keyspace/Database/KeyspaceDB.h"

class BackupServer : public TCPServerT<BackupServer, BackupConn>, public KeyspaceDB
{
public:
	void				InitServer(KeyspaceDB* kdb, int port);
	
	// KeyspaceDB interface
	virtual bool		Init();
	virtual void		Shutdown();
	virtual bool		Add(KeyspaceOp* op);
	virtual bool		Submit();
	virtual unsigned	GetNodeID();
	virtual bool		IsMasterKnown();
	virtual int			GetMaster();
	virtual bool		IsMaster();
	virtual bool		IsReplicated();
	virtual void		SetProtocolServer(ProtocolServer* pserver);
	
	uint64_t			GetLastID();
	uint64_t			GetLastKnownID();
	
	void				RegisterBackupConn(BackupConn* conn);
	
private:
	KeyspaceDB*			kdb;
	List<BackupConn*>	activeConns;
	uint64_t			cmdID;
};

#endif
