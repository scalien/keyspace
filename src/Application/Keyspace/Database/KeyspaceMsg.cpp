#include "KeyspaceMsg.h"
#include "KeyspaceService.h"

void KeyspaceMsg::Init(char type_)
{
	type = type_;
}

void KeyspaceMsg::Set(ByteString key_, ByteString value_)
{
	Init(KEYSPACE_SET);
	
	key.Set(key_);
	value.Set(value_);
}
	
void KeyspaceMsg::TestAndSet(ByteString key_, ByteString test_, ByteString value_)
{
	Init(KEYSPACE_TEST_AND_SET);
	
	key.Set(key_);
	test.Set(test_);
	value.Set(value_);
}

void KeyspaceMsg::Add(ByteString key_, int64_t num_)
{
	Init(KEYSPACE_ADD);
	
	key.Set(key_);
	num = num_;
}

void KeyspaceMsg::Rename(ByteString key_, ByteString newKey_)
{
	Init(KEYSPACE_RENAME);
	key.Set(key_);
	newKey.Set(newKey_);
}


void KeyspaceMsg::Delete(ByteString key_)
{
	Init(KEYSPACE_DELETE);
	
	key.Set(key_);
}

void KeyspaceMsg::Prune(ByteString prefix_)
{
	Init(KEYSPACE_PRUNE);
	
	prefix.Set(prefix_);
}
	
bool KeyspaceMsg::Read(ByteString& data, unsigned &n)
{
	int read;
	
	if (data.length < 1)
		return false;
	
	switch (data.buffer[0])
	{
		case KEYSPACE_SET:
			read = snreadf(data.buffer, data.length, "%c:%M:%M",
						   &type, &key, &value);
			break;
		case KEYSPACE_TEST_AND_SET:
			read = snreadf(data.buffer, data.length, "%c:%M:%M:%M",
						   &type, &key, &test, &value);
			break;
		case KEYSPACE_ADD:
			read = snreadf(data.buffer, data.length, "%c:%M:%I",
						   &type, &key, &num);
			break;
		case KEYSPACE_RENAME:
			read = snreadf(data.buffer, data.length, "%c:%M:%M",
						   &type, &key, &newKey);
			break;
		case KEYSPACE_DELETE:
			read = snreadf(data.buffer, data.length, "%c:%M",
						   &type, &key);
			break;
		case KEYSPACE_PRUNE:
			read = snreadf(data.buffer, data.length, "%c:%M",
						   &type, &prefix);
			break;
		default:
			return false;
	}
	
	if (read < 0)
		return false;
	
	n = read;
	
	return true;
}

bool KeyspaceMsg::Write(ByteString& data)
{
	switch (type)
	{
		case KEYSPACE_SET:
			return data.Writef("%c:%M:%M",
						       type, &key, &value);
			break;
		case KEYSPACE_TEST_AND_SET:
			return data.Writef("%c:%M:%M:%M",
						       type, &key, &test, &value);
			break;
		case KEYSPACE_ADD:
			return data.Writef("%c:%M:%I",
						       type, &key, num);
			break;
		case KEYSPACE_RENAME:
			return data.Writef("%c:%M:%M",
							   type, &key, &newKey);
			break;
		case KEYSPACE_DELETE:
			return data.Writef("%c:%M",
							   type, &key);
			break;
		case KEYSPACE_PRUNE:
			return data.Writef("%c:%M",
							   type, &prefix);
			break;
		default:
			return false;
	}
}

bool KeyspaceMsg::FromKeyspaceOp(KeyspaceOp* op)
{
	bool ret;
	
	if (op->type == KeyspaceOp::SET)
		Init(KEYSPACE_SET);
	else if (op->type == KeyspaceOp::TEST_AND_SET)
		Init(KEYSPACE_TEST_AND_SET);
	else if (op->type == KeyspaceOp::ADD)
		Init(KEYSPACE_ADD);
	else if (op->type == KeyspaceOp::RENAME)
		Init(KEYSPACE_RENAME);
	else if (op->type == KeyspaceOp::DELETE)
		Init(KEYSPACE_DELETE);
	else if (op->type == KeyspaceOp::PRUNE)
		Init(KEYSPACE_PRUNE);
	else
		ASSERT_FAIL();
	
	ret = true;
	
	if (op->type == KeyspaceOp::PRUNE)
		ret &= prefix.Set(op->prefix);
	else
		ret &= key.Set(op->key);
	
	if (op->type == KeyspaceOp::RENAME)
		ret &= newKey.Set(op->newKey);
	
	if (op->type == KeyspaceOp::SET || op->type == KeyspaceOp::TEST_AND_SET)
		ret &= value.Set(op->value);
	if (op->type == KeyspaceOp::TEST_AND_SET)
		ret &= test.Set(op->test);
	if (op->type == KeyspaceOp::ADD)
		num = op->num;
	return ret;
}
