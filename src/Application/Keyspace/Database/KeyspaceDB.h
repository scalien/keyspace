#ifndef KEYSPACEDB_H
#define KEYSPACEDB_H

class KeyspaceOp;

class KeyspaceClient
{
public:
	virtual	void	OnComplete(KeyspaceOp* op, int status) = 0;
};

class KeyspaceOp
{
public:
	enum Type
	{
		GET,
		SET,
		TEST_AND_SET
	};
	
	Type			type;
	ByteString		key;
	ByteString		value;
	ByteString		test;
	
	KeyspaceClient*	client;
};

class KeyspaceDB
{
public:
	bool Add(KeyspaceOp& op);
};

#endif
