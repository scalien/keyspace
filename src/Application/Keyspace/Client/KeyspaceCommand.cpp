#include "KeyspaceCommand.h"
#include "KeyspaceResponse.h"
#include "KeyspaceClientConsts.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientReq.h"

using namespace Keyspace;

Command::Command()
{
	type = 0;
	nodeID = -1;
	status = KEYSPACE_NOSERVICE;
	cmdID = 0;
}

Command::~Command()
{
	ClearResponse();
}

bool Command::IsDirty() const
{
	switch(type)
	{
	case KEYSPACECLIENT_DIRTY_GET:
	case KEYSPACECLIENT_DIRTY_LIST:
	case KEYSPACECLIENT_DIRTY_LISTP:
	case KEYSPACECLIENT_DIRTY_COUNT:
		return true;

	default:
		return false;
	}
}

bool Command::IsList() const
{
	if (type == KEYSPACECLIENT_LIST ||
		type == KEYSPACECLIENT_LISTP ||
		type == KEYSPACECLIENT_DIRTY_LIST ||
		type == KEYSPACECLIENT_DIRTY_LISTP)
	{
		return true;
	}
	
	return false;
}

void Command::ClearResponse()
{
	Response**	it;
	
	for (it = responses.Head(); it != NULL;)
	{
		delete *it;
		it = responses.Remove(it);
	}
}
