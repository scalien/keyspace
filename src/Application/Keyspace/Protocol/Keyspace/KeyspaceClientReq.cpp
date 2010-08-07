#include "KeyspaceClientReq.h"
#include "Application/Keyspace/Database/KeyspaceService.h"

void KeyspaceClientReq::Init()
{
	key.length = 0;
	newKey.length = 0;
	test.length = 0;
	value.length = 0;
	prefix.length = 0;

	cmdID = 0;
	count = 0;
	offset = 0;
	expiryTime = 0;
	num = 0;
}
	
bool KeyspaceClientReq::Read(const ByteString& data)
{
	int			read;
	unsigned	dummy;
	
	Init();
	
	if (data.length < 1)
		return false;
	
	switch (data.buffer[0])
	{
		case KEYSPACECLIENT_GET_MASTER:
			read = snreadf(data.buffer, data.length, "%c:%U",
						   &type, &cmdID);
			break;
		case KEYSPACECLIENT_GET:
		case KEYSPACECLIENT_DIRTY_GET:
			read = snreadf(data.buffer, data.length, "%c:%U:%N",
						   &type, &cmdID, &key);
			break;
		case KEYSPACECLIENT_LIST:
		case KEYSPACECLIENT_DIRTY_LIST:
		case KEYSPACECLIENT_LISTP:
		case KEYSPACECLIENT_DIRTY_LISTP:
		case KEYSPACECLIENT_COUNT:
		case KEYSPACECLIENT_DIRTY_COUNT:
			read = snreadf(data.buffer, data.length,
						   "%c:%U:%N:%N:%u:%U:%u:%U:%u:%c",
						   &type, &cmdID, &prefix, &key,
						   &dummy, &count, &dummy, &offset,
						   &dummy, &direction);
			if (direction != 'f' && direction != 'b')
				return false;
			break;
		case KEYSPACECLIENT_SET:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%N",
						   &type, &cmdID, &key, &value);
			break;
		case KEYSPACECLIENT_TEST_AND_SET:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%N:%N",
						   &type, &cmdID, &key, &test, &value);
			break;
		case KEYSPACECLIENT_DELETE:
		case KEYSPACECLIENT_REMOVE:
			read = snreadf(data.buffer, data.length, "%c:%U:%N",
						   &type, &cmdID, &key);
			break;
		case KEYSPACECLIENT_PRUNE:
			read = snreadf(data.buffer, data.length, "%c:%U:%N",
						   &type, &cmdID, &prefix);
			break;
		case KEYSPACECLIENT_ADD:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%u:%I",
						   &type, &cmdID, &key, &dummy, &num);
			break;
		case KEYSPACECLIENT_RENAME:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%N",
						   &type, &cmdID, &key, &newKey);
			break;
		case KEYSPACECLIENT_SET_EXPIRY:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%u:%U",
						   &type, &cmdID, &key, &dummy, &expiryTime);
			break;
		case KEYSPACECLIENT_SUBMIT:
			read = snreadf(data.buffer, data.length, "%c", &type);
			break;
		default:
			return false;
	}
	
#define VALIDATE_KEYLEN(bs) { if (bs.length > KEYSPACE_KEY_SIZE) return false; }
#define VALIDATE_VALLEN(bs) { if (bs.length > KEYSPACE_VAL_SIZE) return false; }

	VALIDATE_KEYLEN(key);
	VALIDATE_KEYLEN(newKey);
	VALIDATE_KEYLEN(prefix);
	VALIDATE_VALLEN(test);
	VALIDATE_VALLEN(value);

#undef VALIDATE_KEYLEN
#undef VALIDATE_VALLEN
			
	return (read == (signed)data.length ? true : false);
}

bool KeyspaceClientReq::ToKeyspaceOp(KeyspaceOp* op)
{
	switch (type)
	{
		case KEYSPACECLIENT_GET:
			op->type = KeyspaceOp::GET;
			break;
		case KEYSPACECLIENT_DIRTY_GET:
			op->type = KeyspaceOp::DIRTY_GET;
			break;
		case KEYSPACECLIENT_LIST:
			op->type = KeyspaceOp::LIST;
			op->count = count;
			op->offset = offset;
			op->forward = (direction == 'f');
			break;
		case KEYSPACECLIENT_DIRTY_LIST:
			op->type = KeyspaceOp::DIRTY_LIST;
			op->count = count;
			op->offset = offset;
			op->forward = (direction == 'f');
			break;
		case KEYSPACECLIENT_LISTP:
			op->type = KeyspaceOp::LISTP;
			op->count = count;
			op->offset = offset;
			op->forward = (direction == 'f');
			break;
		case KEYSPACECLIENT_DIRTY_LISTP:
			op->type = KeyspaceOp::DIRTY_LISTP;
			op->count = count;
			op->offset = offset;
			op->forward = (direction == 'f');
			break;
		case KEYSPACECLIENT_COUNT:
			op->type = KeyspaceOp::COUNT;
			op->count = count;
			op->offset = offset;
			op->forward = (direction == 'f');
			break;
		case KEYSPACECLIENT_DIRTY_COUNT:
			op->type = KeyspaceOp::DIRTY_COUNT;
			op->count = count;
			op->offset = offset;
			op->forward = (direction == 'f');
			break;
		case KEYSPACECLIENT_SET:
			op->type = KeyspaceOp::SET;
			break;
		case KEYSPACECLIENT_TEST_AND_SET:
			op->type = KeyspaceOp::TEST_AND_SET;
			break;
		case KEYSPACECLIENT_DELETE:
			op->type = KeyspaceOp::DELETE;
			break;
		case KEYSPACECLIENT_REMOVE:
			op->type = KeyspaceOp::REMOVE;
			break;
		case KEYSPACECLIENT_PRUNE:
			op->type = KeyspaceOp::PRUNE;
			break;
		case KEYSPACECLIENT_ADD:
			op->type = KeyspaceOp::ADD;
			op->num = num;
			break;
		case KEYSPACECLIENT_RENAME:
			op->type = KeyspaceOp::RENAME;
			break;
		case KEYSPACECLIENT_SET_EXPIRY:
			op->type = KeyspaceOp::SET_EXPIRY;
			op->expiryTime = expiryTime;
			break;
		default:
			return false;
	}

	op->cmdID = cmdID;

#define VALIDATE_KEYLEN(bs) { if (bs.length > KEYSPACE_KEY_SIZE) return false; }
#define VALIDATE_VALLEN(bs) { if (bs.length > KEYSPACE_VAL_SIZE) return false; }

	VALIDATE_KEYLEN(key);
	VALIDATE_KEYLEN(newKey);
	VALIDATE_KEYLEN(prefix);
	VALIDATE_VALLEN(test);
	VALIDATE_VALLEN(value);

#undef VALIDATE_KEYLEN
#undef VALIDATE_VALLEN
	
	if (!op->key.Set(key)) return false;
	if (!op->newKey.Set(newKey)) return false;
	if (!op->test.Set(test)) return false;
	if (!op->value.Set(value)) return false;
	if (!op->prefix.Set(prefix)) return false;

	return true;
}

bool KeyspaceClientReq::IsRead()
{
	if (type == KEYSPACECLIENT_GET_MASTER ||
	type == KEYSPACECLIENT_GET ||
	type == KEYSPACECLIENT_DIRTY_GET ||
	type == KEYSPACECLIENT_LIST ||
	type == KEYSPACECLIENT_DIRTY_LIST ||
	type == KEYSPACECLIENT_LISTP ||
	type == KEYSPACECLIENT_DIRTY_LISTP ||
	type == KEYSPACECLIENT_COUNT ||
	type == KEYSPACECLIENT_DIRTY_COUNT)
		return true;

	return false;
}


bool KeyspaceClientReq::IsWrite()
{
	return !IsRead();
}

bool KeyspaceClientReq::IsDirty()
{
	if (type == KEYSPACECLIENT_DIRTY_GET ||
	type == KEYSPACECLIENT_DIRTY_LIST ||
	type == KEYSPACECLIENT_DIRTY_LISTP ||
	type == KEYSPACECLIENT_DIRTY_COUNT)
		return true;
	
	return false;
}
