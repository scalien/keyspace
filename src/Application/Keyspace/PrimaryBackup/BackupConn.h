#ifndef BACKUP_CONN_H
#define BACKUP_CONN_H

#include "Framework/Transport/MessageConn.h"
#include "Application/Keyspace/Database/KeyspaceConsts.h"
#include "Application/Keyspace/Database/KeyspaceMsg.h"

class BackupServer;

class BackupConn : public MessageConn<>
{
typedef ByteArray<KEYSPACE_KEY_SIZE> KeyBuffer;
typedef ByteArray<KEYSPACE_VAL_SIZE> ValBuffer;

enum State
{
	HANDSHAKE,
	DUMP,
	CATCHUP,
	SYNC
};

enum Error
{
	PROTOCOL,
	UUID,
	CMDID,
	RATE,
};

public:
	void				Init(BackupServer* server);

	virtual void		OnMessageRead(const ByteString& msg);
	virtual void		OnClose();
	virtual void		OnWrite();

	State				GetState() const;

	void				WriteKeyspaceMsg(const KeyspaceMsg& msg);

private:
	BackupServer*		server;
	State				state;
	bool				closeAfterSend;
	
	void				WriteError(Error e);
};

#endif
