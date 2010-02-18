#ifndef KEYSPACE_RESPONSE_H
#define KEYSPACE_RESPONSE_H

#include "System/Buffer.h"

namespace Keyspace
{

class Client;
class Command;

class Response
{
public:
	DynArray<128>	key;
	DynArray<128>	value;
	int				commandStatus;
	char			type;
	uint64_t		id;
	
	bool			Read(const ByteString& data);
	
private:
	char*			pos;
	char			separator;
	ByteString		data;
	ByteString		msg;
	
	bool			CheckOverflow();
	bool			ReadUint64(uint64_t& num);
	bool			ReadChar(char& c);
	bool			ReadSeparator();
	bool			ReadMessage(ByteString& bs);
	bool			ReadData(ByteString& bs, uint64_t length);

	bool			ValidateLength();
};

}; // namespace

#endif
