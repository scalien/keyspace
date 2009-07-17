#ifndef KEYSPACEDB_H
#define KEYSPACEDB_H

#include "KeyspaceConsts.h"
#include "../Protocol/ProtocolServer.h"

class KeyspaceOp;

class KeyspaceDB
{
public:
	virtual ~KeyspaceDB() {}
	
	virtual bool		Init() = 0;
	virtual void		Shutdown() = 0;
	virtual bool		Add(KeyspaceOp* op) = 0;
	virtual bool		Submit() = 0;
	virtual unsigned	GetNodeID() = 0;
	virtual bool		IsMasterKnown() = 0;
	virtual int			GetMaster() = 0;
	virtual bool		IsMaster() = 0;
	virtual void		SetProtocolServer(ProtocolServer* pserver) = 0;
};


#endif
