#ifndef KEYSPACEDB_H
#define KEYSPACEDB_H

class Client
{
public:
	virtual	void	OnComplete(KeyspaceOp* op) = 0;
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
	
	Type		type;
	ByteString	key;
	ByteString	value;
	ByteString	test;
	
	Client*		client;
};

class KeyspaceDB
{
public:
	bool Add(KeyspaceOp& op);
};

#endif
