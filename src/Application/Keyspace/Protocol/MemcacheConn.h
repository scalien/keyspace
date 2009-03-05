#ifndef MEMCACHE_CONN_H
#define MEMCACHE_CONN_H

#include "System/Containers/List.h"
#include "System/Events/Callable.h"
#include "System/IO/Socket.h"
#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"

#include "../Database/KeyspaceDB.h"

class MemcacheServer;

class MemcacheConn : public KeyspaceClient
{
public:
	typedef ByteArray<4096> WriteBuffer;
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
	List<WriteBuffer*> writeQueue;
	int				numpending;
	bool			closed;
	KeyspaceDB*		kdb;
	const char*		newline;
	
	MemcacheConn();
	~MemcacheConn();
	
	void			Init(IOProcessor* ioproc_, KeyspaceDB* kdb_, MemcacheServer* server_);
	void			Write(const char *data, int size);
	void			OnRead();
	void			OnWrite();
	void			OnClose();
	
	MFunc<MemcacheConn>	onRead;
	MFunc<MemcacheConn>	onWrite;
	MFunc<MemcacheConn>	onClose;

	// KeyspaceClient interface
	virtual void	OnComplete(KeyspaceOp* op, bool status);
	
private:
	int			Tokenize(const char* data, int size, Token* tokens, int maxtokens);
	const char* Process(const char* data, int size);
	const char* ProcessGetCommand(const char* data, int size, Token* tokens, int numtoken);
	const char* ProcessSetCommand(const char* data, int size, Token* tokens, int numtoken);
	void		Add(KeyspaceOp& op);
	void		TryClose();
	void		WritePending();
};

#endif
