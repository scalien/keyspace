#ifndef KEYSPACE_RESULT_H
#define KEYSPACE_RESULT_H

#include "System/Containers/List.h"
#include "System/Buffer.h"

namespace Keyspace
{

class ClientConn;
class Client;
class Command;
class Response;

typedef List<Command*>	CommandList;

class Result
{
friend class ClientConn;
friend class Client;

public:	
	~Result();
	
	void				Close();

	void				Begin();
//	void				End();
	
	void				Next();
//	void				Prev();

//	bool				IsBegin();
	bool				IsEnd();
	
	int					Key(ByteString& key) const;
	int					Value(ByteString& value) const;
	
	int					CommandStatus() const;
	int					GetNodeID() const;

	int					TransportStatus() const;
	int					ConnectivityStatus() const;
	int					TimeoutStatus() const;
	
private:
	bool				isBatched;
	CommandList			commands;
	int					numCompleted;
	ByteString			empty;
	int					transportStatus;
	int					connectivityStatus;
	int					timeoutStatus;
	Command**			commandCursor;
	Response**			responseCursor;
	
	Command**			cmdMap;


	Result();

	void				AppendCommandResponse(Command* cmd, Response* resp);
	void				AppendCommand(Command* cmd);

	int					ListKey(Command* cmd, ByteString& key) const;
	int					ListValue(Command* cmd, ByteString& value) const;
	
	void				InitCommandMap();
	void				FreeCommandMap();	
};



}; // namespace

#endif
