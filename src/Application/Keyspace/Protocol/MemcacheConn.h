#ifndef MEMCACHE_CONN_H
#define MEMCACHE_CONN_H

#include "System/Events/Callable.h"
#include "System/IO/Socket.h"
#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"

#include "../Database/KeyspaceDB.h"

class MemcacheServer;

class MemcacheConn : public KeyspaceClient
{
public:
	class Token
	{
	public:
		const char*	value;
		int			len;
	};
	
	Socket			socket;
	TCPRead			tcpread;
	TCPWrite		tcpwrite;
	IOProcessor*	ioproc;
	MemcacheServer*	server;
	ByteArray<1024>	readBuffer;
	ByteArray<1024>	writeBuffer;
	int				numpending;
	bool			closed;
	KeyspaceDB*		kdb;
	
	MemcacheConn();
	
	void			Init(MemcacheServer* server_, IOProcessor* ioproc_, KeyspaceDB* kdb_);
	void			Write(const char *data, int size);
	void			OnRead();
	void			OnWrite();
	void			OnClose();
	
	MFunc<MemcacheConn>	onRead;
	MFunc<MemcacheConn>	onWrite;
	MFunc<MemcacheConn>	onClose;

	// KeyspaceClient interface
	virtual void	OnComplete(KeyspaceOp* op, int status);
	
private:
	int			Tokenize(const char* data, int size, Token* tokens, int maxtokens);
	const char* Process(const char* data, int size);
	const char* ProcessGetCommand(const char* data, int size, Token* tokens, int numtoken);
	const char* ProcessSetCommand(const char* data, int size, Token* tokens, int numtoken);
	void		Add(KeyspaceOp& op);
	void		TryClose();
};

#endif
