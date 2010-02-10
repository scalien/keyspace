#include "KeyspaceResult.h"
#include "KeyspaceClientConsts.h"
#include "KeyspaceCommand.h"
#include "KeyspaceResponse.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientReq.h"

using namespace Keyspace;

Result::Result()
{
	Close();
}

Result::~Result()
{
	Close();
}

void Result::Close()
{
	Command**	it;
	
	isBatched = false;
	numCompleted = 0;
	transportStatus = KEYSPACE_FAILURE;
	
	commandCursor = NULL;
	responseCursor = NULL;
	
	while ((it = commands.Head()) != NULL)
	{
		delete *it;
		commands.Remove(it);
	}
}

void Result::Begin()
{
	commandCursor = commands.Head();
	if (commandCursor && (*commandCursor)->IsList())
		responseCursor = (*commandCursor)->responses.Head();
}

//void Result::End()
//{
//	commandCursor = commands.Tail();
//	if (commandCursor && (*commandCursor)->IsList())
//		responseCursor = (*commandCursor)->responses.Tail();
//}

void Result::Next()
{
	if (responseCursor)
		responseCursor = (*commandCursor)->responses.Next(responseCursor);
	
	commandCursor = commands.Next(commandCursor);
}

//void Result::Prev()
//{
//	if (responseCursor)
//		responseCursor = (*commandCursor)->responses.Prev(responseCursor);
//	
//	commandCursor = commands.Prev(commandCursor);
//}

//bool Result::IsBegin()
//{
//	if (commandCursor && (*commandCursor)->IsList())
//		return (responseCursor == NULL);
//	
//	return (commandCursor == NULL);
//}

bool Result::IsEnd()
{
	if (commandCursor && (*commandCursor)->IsList())
		return (responseCursor == NULL);
	
	return (commandCursor == NULL);
}

int Result::Key(ByteString& key) const
{
	Command*	cmd;
	
	if (commandCursor == NULL)
		return KEYSPACE_API_ERROR;
	
	cmd = *commandCursor;
	if (cmd->IsList())
		return ListKey(cmd, key);
	
	key = cmd->key;
	
	return cmd->status;
}

int Result::Value(ByteString& value) const
{
	Command*	cmd;
	Response**	rit;
	Response*	resp;
	
	if (commandCursor == NULL)
		return KEYSPACE_API_ERROR;
	
	cmd = *commandCursor;
	if (cmd->IsList())
		return ListValue(cmd, value);
	
	rit = cmd->responses.Head();
	if (!rit)
		return KEYSPACE_NOSERVICE;
	
	resp = *rit;
	
	if (cmd->type == KEYSPACECLIENT_GET ||
		cmd->type == KEYSPACECLIENT_DIRTY_GET ||
		cmd->type == KEYSPACECLIENT_COUNT ||
		cmd->type == KEYSPACECLIENT_DIRTY_COUNT ||
		cmd->type == KEYSPACECLIENT_ADD ||
		cmd->type == KEYSPACECLIENT_REMOVE ||
		cmd->type == KEYSPACECLIENT_TEST_AND_SET)
	{
		if (cmd->status == KEYSPACE_SUCCESS)
			value = resp->value;
	}

	return cmd->status;
}

int Result::ListKey(Command* cmd, ByteString& key) const
{
	Response*	resp;

	if (responseCursor == NULL)
		return cmd->status;

	resp = *responseCursor;
	key = resp->key;
	
	return KEYSPACE_SUCCESS;
}

int Result::ListValue(Command* cmd, ByteString& value) const
{
	Response*	resp;

	if (responseCursor == NULL)
		return cmd->status;

	resp = *responseCursor;
	value = resp->value;
	
	return KEYSPACE_SUCCESS;
}

int Result::GetNodeID() const
{
	if (commandCursor)
		return (*commandCursor)->nodeID;
	
	return KEYSPACE_API_ERROR;
}

int Result::CommandStatus() const
{
	if (commandCursor)
		return (*commandCursor)->status;
	
	return KEYSPACE_API_ERROR;
}		

int Result::TransportStatus() const
{
	return transportStatus;
}

int Result::ConnectivityStatus() const
{
	return connectivityStatus;
}

int Result::TimeoutStatus() const
{
	return timeoutStatus;
}

void Result::AppendCommandResponse(Command* cmd, Response* resp_)
{
	assert(!cmd->IsList() && cmd->responses.Length() > 0);
	
	transportStatus = KEYSPACE_PARTIAL;
	cmd->responses.Append(resp_);
}

void Result::AppendCommand(Command* cmd)
{
	commands.Append(cmd);
}

