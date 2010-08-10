#include "KeyspaceMsg.h"
#include "KeyspaceService.h"

void KeyspaceMsg::Init(char type_)
{
	type = type_;
}

//void KeyspaceMsg::Set(ByteString key_, ByteString value_)
//{
//	Init(KEYSPACE_SET);
//	key.Set(key_);
//	value.Set(value_);
//}
//	
//void KeyspaceMsg::TestAndSet(ByteString key_,
//ByteString test_, ByteString value_)
//{
//	Init(KEYSPACE_TEST_AND_SET);
//	key.Set(key_);
//	test.Set(test_);
//	value.Set(value_);
//}
//
//void KeyspaceMsg::Add(ByteString key_, int64_t num_)
//{
//	Init(KEYSPACE_ADD);
//	key.Set(key_);
//	num = num_;
//}
//
//void KeyspaceMsg::Rename(ByteString key_, ByteString newKey_)
//{
//	Init(KEYSPACE_RENAME);
//	key.Set(key_);
//	newKey.Set(newKey_);
//}
//
//void KeyspaceMsg::Delete(ByteString key_)
//{
//	Init(KEYSPACE_DELETE);	
//	key.Set(key_);
//}
//
//void KeyspaceMsg::Remove(ByteString key_)
//{
//	Init(KEYSPACE_REMOVE);	
//	key.Set(key_);
//}
//
//void KeyspaceMsg::Prune(ByteString prefix_)
//{
//	Init(KEYSPACE_PRUNE);
//	prefix.Set(prefix_);
//}
//
//void KeyspaceMsg::SetExpiry(ByteString key_, uint64_t expiryTime_)
//{
//	Init(KEYSPACE_SET_EXPIRY);
//	key.Set(key_);
//	expiryTime = expiryTime_;
//}
//	
//void KeyspaceMsg::Expire(ByteString key_, uint64_t expiryTime_)
//{
//	Init(KEYSPACE_EXPIRE);
//	key.Set(key_);
//	expiryTime = expiryTime_;
//}
//
//void KeyspaceMsg::RemoveExpiry(ByteString key_, uint64_t expiryTime_)
//{
//	Init(KEYSPACE_REMOVE_EXPIRY);
//	key.Set(key_);
//	expiryTime = expiryTime_;
//}

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
		case KEYSPACE_REMOVE:
			read = snreadf(data.buffer, data.length, "%c:%M",
						   &type, &key);
			break;
		case KEYSPACE_PRUNE:
			read = snreadf(data.buffer, data.length, "%c:%M",
						   &type, &prefix);
			break;
		case KEYSPACE_SET_EXPIRY:
			read = snreadf(data.buffer, data.length, "%c:%M:%U:%U",
						   &type, &key, &prevExpiryTime, &nextExpiryTime);
			break;
		case KEYSPACE_EXPIRE:
			read = snreadf(data.buffer, data.length, "%c:%M:%U",
						   &type, &key, &prevExpiryTime);
			break;
		case KEYSPACE_REMOVE_EXPIRY:
			read = snreadf(data.buffer, data.length, "%c:%M:%U",
						   &type, &key, &prevExpiryTime);
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
		case KEYSPACE_REMOVE:
			return data.Writef("%c:%M",
							   type, &key);
			break;
		case KEYSPACE_PRUNE:
			return data.Writef("%c:%M",
							   type, &prefix);
			break;
		case KEYSPACE_SET_EXPIRY:
			return data.Writef("%c:%M:%U:%U",
						       type, &key, prevExpiryTime, nextExpiryTime);
			break;
		case KEYSPACE_EXPIRE:
			return data.Writef("%c:%M:%U",
						       type, &key, prevExpiryTime);
			break;
		case KEYSPACE_REMOVE_EXPIRY:
			return data.Writef("%c:%M:%U",
						       type, &key, prevExpiryTime);
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
	else if (op->type == KeyspaceOp::REMOVE)
		Init(KEYSPACE_REMOVE);
	else if (op->type == KeyspaceOp::PRUNE)
		Init(KEYSPACE_PRUNE);
	else if (op->type == KeyspaceOp::SET_EXPIRY)
		Init(KEYSPACE_SET_EXPIRY);	
	else if (op->type == KeyspaceOp::EXPIRE)
		Init(KEYSPACE_EXPIRE);
	else if (op->type == KeyspaceOp::REMOVE_EXPIRY)
		Init(KEYSPACE_REMOVE_EXPIRY);
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
	if (op->type == KeyspaceOp::SET_EXPIRY)
	{
		prevExpiryTime = op->prevExpiryTime;
		nextExpiryTime = op->nextExpiryTime;
	}
	if (op->type == KeyspaceOp::EXPIRE ||
		op->type == KeyspaceOp::REMOVE_EXPIRY)
	{
		prevExpiryTime = op->prevExpiryTime;
	}
		
	return ret;
}
