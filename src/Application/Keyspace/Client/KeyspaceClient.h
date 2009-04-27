#ifndef KEYSPACECLIENT_H
#define KEYSPACECLIENT_H

#include "System/IO/Endpoint.h"
#include "System/IO/Socket.h"
#include "System/Buffer.h"

#define KEYSPACE_OK			0
#define KEYSPACE_NOTMASTER	-1
#define KEYSPACE_FAILED		-2
#define KEYSPACE_ERROR		-3

class KeyspaceClient
{
public:
	class Result
	{
	public:
		friend class KeyspaceClient;
		
		Result(KeyspaceClient &client);
		~Result();
		
		void				Close();
		Result*				Next(int &status);

		const ByteString&	Key();
		const ByteString&	Value();
		int					Status();

	private:
		KeyspaceClient		&client;
		uint64_t			id;
		int					type;
		DynArray<128>		key;
		DynArray<128>		value;
		int					status;
		
		int					ParseValueResponse(const ByteString &resp);
		int					ParseListResponse(const ByteString &resp);
		int					ParseListPResponse(const ByteString &resp);
	};
	
	KeyspaceClient(int nodec, char* nodev[], uint64_t timeout);
	~KeyspaceClient();
	
	// master connection related commands
	int				ConnectMaster();
	int				GetMaster();
	
	// simple get commands with preallocated value
	int				Get(const ByteString &key, ByteString &value, bool dirty = false);
	int				DirtyGet(const ByteString &key, ByteString &value);

	// commands that return a Result
	int				Get(const ByteString &key, bool dirty = false);
	int				DirtyGet(const ByteString &key);
	int				List(const ByteString &prefix, uint64_t count = 0, bool dirty = false);
	int				DirtyList(const ByteString &prefix, uint64_t count = 0);
	int				ListP(const ByteString &prefix, uint64_t count = 0, bool dirty = false);
	int				DirtyListP(const ByteString &prefix, uint64_t count = 0);

	Result*			GetResult(int &status);

	// write commands
	int				Set(const ByteString &key, const ByteString &value, bool sumbit = true);
	int				TestAndSet(const ByteString &key, const ByteString &test, const ByteString &value, 
							   bool submit = true);
	int				Add(const ByteString &key, int64_t num, int64_t &result, bool submit = true);
	int				Delete(const ByteString &key, bool submit = true);

	// grouping write commands
	int				Begin();
	int				Submit();

private:
	friend class Result;
	
	bool			connectMaster;
	int				numEndpoints;
	Endpoint*		endpoints;
	Endpoint*		endpoint;
	uint64_t		timeout;
	uint64_t		id;
	uint64_t		startId;
	int				numPending;
	Socket			socket;
	DynArray<4096>	readBuf;
	Result			result;
		
	uint64_t		GetNextID();
	void			Reconnect();
	bool			ConnectRandom();
	bool			Connect(int n);
	void			Disconnect();
	void			SendMessage(char cmd, bool submit, int msgc, const ByteString *msgv);
	void			Send(const ByteString &msg);
	void			ResetReadBuffer();
	int				Read(ByteString &msg);
	int				ReadMessage(ByteString &msg);

	int				GetValueResponse(ByteString &resp);
	int				GetStatusResponse();
};

#endif
