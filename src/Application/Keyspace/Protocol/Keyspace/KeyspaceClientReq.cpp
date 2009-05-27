#include "KeyspaceClientReq.h"
#include "Application/Keyspace/Database/KeyspaceService.h"

void KeyspaceClientReq::Init()
{
	key.length = 0;
	test.length = 0;
	value.length = 0;
	prefix.length = 0;
}
	
bool KeyspaceClientReq::Read(const ByteString& data)
{
	int read;		// %u:%u and %I:%I is stupid
	
	if (data.length < 1)
		return false;
	
	switch (data.buffer[0])
	{
		case KEYSPACECLIENT_GET_MASTER:
			read = snreadf(data.buffer, data.length, "%c:%U",
						   &type, &cmdID);
			break;
		case KEYSPACECLIENT_GET:
			read = snreadf(data.buffer, data.length, "%c:%U:%N",
						   &type, &cmdID, &key);
			break;
		case KEYSPACECLIENT_DIRTY_GET:
			read = snreadf(data.buffer, data.length, "%c:%U:%N",
						   &type, &cmdID, &key);
			break;
		case KEYSPACECLIENT_LIST:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%u:%u",
						   &type, &cmdID, &prefix, &count, &count);
			break;
		case KEYSPACECLIENT_DIRTY_LIST:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%u:%u",
						   &type, &cmdID, &prefix, &count, &count);
			break;
		case KEYSPACECLIENT_LISTP:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%u:%u",
						   &type, &cmdID, &prefix, &count, &count);
			break;
		case KEYSPACECLIENT_DIRTY_LISTP:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%u:%u",
						   &type, &cmdID, &prefix, &count, &count);
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
			read = snreadf(data.buffer, data.length, "%c:%U:%N",
						   &type, &cmdID, &key);
			break;
		case KEYSPACECLIENT_PRUNE:
			read = snreadf(data.buffer, data.length, "%c:%U:%N",
						   &type, &cmdID, &prefix);
			break;
		case KEYSPACECLIENT_ADD:
			read = snreadf(data.buffer, data.length, "%c:%U:%N:%I:%I",
						   &type, &cmdID, &key, &num, &num);
			break;
		case KEYSPACECLIENT_SUBMIT:
			read = snreadf(data.buffer, data.length, "%c", &type);
			break;
		default:
			return false;
	}
			
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
			break;
		case KEYSPACECLIENT_DIRTY_LIST:
			op->type = KeyspaceOp::DIRTY_LIST;
			op->count = count;
			break;
		case KEYSPACECLIENT_LISTP:
			op->type = KeyspaceOp::LISTP;
			op->count = count;
			break;
		case KEYSPACECLIENT_DIRTY_LISTP:
			op->type = KeyspaceOp::DIRTY_LISTP;
			op->count = count;
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
		case KEYSPACECLIENT_PRUNE:
			op->type = KeyspaceOp::PRUNE;
			break;
		case KEYSPACECLIENT_ADD:
			op->type = KeyspaceOp::ADD;
			op->num = num;
			break;
		default:
			return false;
	}

	op->cmdID = cmdID;

	if (!op->key.Set(key)) return false;
	if (!op->test.Set(test)) return false;
	if (!op->value.Set(value)) return false;
	if (!op->prefix.Set(prefix)) return false;

	return true;
}
