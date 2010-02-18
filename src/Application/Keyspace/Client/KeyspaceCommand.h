#ifndef KEYSPACE_COMMAND_H
#define KEYSPACE_COMMAND_H

#include "System/Buffer.h"
#include "System/Containers/List.h"

namespace Keyspace
{

class Response;
typedef List<Response*> ResponseList;

class Command
{
public:
	Command();
	~Command();
	
	bool				IsDirty() const;
	bool				IsList() const;
	bool				IsRead() const;
	bool				IsWrite() const;

	void				ClearResponse();

	char				type;
	ByteString			key;
	DynArray<128>		args;
	int					nodeID;
	int					status;
	uint64_t			cmdID;
	
	ResponseList		responses;
};

}; // namespace

#endif
