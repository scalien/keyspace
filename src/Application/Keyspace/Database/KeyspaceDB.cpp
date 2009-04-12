#include "Framework/AsyncDatabase/AsyncDatabase.h"
#include "KeyspaceDB.h"
#include "KeyspaceClient.h"
#include <assert.h>
#include <stdlib.h>
#include "System/Log.h"
#include "System/Common.h"

class ListVisitor : public TableVisitor
{
public:
	ListVisitor(const ByteString &keyHint_) :
	keyHint(keyHint_)
	{}
	
	virtual bool Accept(const ByteString &key, const ByteString &)
	{
		// don't list system keys
		if (key.length >= 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
			return true;

		out.Append(key.buffer, key.length);
		out.Append("\n", 1);
		return true;
	}
	
	virtual const ByteString* GetKeyHint()
	{
		return &keyHint;
	}
	
	ByteString &GetOutput()
	{
		return out;
	}
	
private:
	const ByteString	&keyHint;
	DynArray<1024>		out;
};

#define VISITOR_LIMIT	1024
class AsyncVisitorCallback : public Callable
{
public:
	typedef ByteArray<VISITOR_LIMIT> Buffer; // TODO better initial value
	ByteString*		keys;
	int				numkey;
	Buffer*			buffer;
	KeyspaceOp*		op;
	bool			complete;
	
	AsyncVisitorCallback()
	{
		keys = new ByteString[VISITOR_LIMIT];
		buffer = new Buffer;
		numkey = 0;
		op = NULL;
		complete = false;
	}
	
	~AsyncVisitorCallback()
	{
		delete[] keys;
		delete buffer;	
	}
	
	bool AppendKey(const ByteString &key)
	{
		if (key.length + buffer->length <= buffer->size && numkey < VISITOR_LIMIT - 1)
		{
			keys[numkey].length = key.length;
			keys[numkey].size = key.size;
			keys[numkey].buffer = buffer->buffer + buffer->length;
			memcpy(buffer->buffer + buffer->length, key.buffer, key.length);
			buffer->length += key.length;
			
			numkey++;
			return true;
		}
		
		return false;
	}
	
	void Execute()
	{
		for (int i = 0; i < numkey; i++)
		{
			if (op->key.size < keys[i].length)
			{
				op->key.Free();
				op->key.Allocate(keys[i].length);
			}
			op->key.Set(keys[i].buffer, keys[i].length);
			if (i == numkey - 1)
				op->client->OnComplete(op, true, complete);
			else
				op->client->OnComplete(op, true, false);
		}

		if (numkey == 0 && complete)
		{
			op->key.length = 0;
			op->client->OnComplete(op, true, complete);
		}
				
		delete this;
	}
};

class AsyncListVisitor : public TableVisitor
{
public:
	AsyncListVisitor(const ByteString &keyHint_, KeyspaceOp* op_, uint64_t count_) :
	keyHint(keyHint_)
	{
		op = op_;
		count = count_;
		num = 0;
		Init();
	}
	
	void Init()
	{
		avc = new AsyncVisitorCallback;
		avc->op = op;
	}
	
	void AppendKey(const ByteString &key)
	{
		if (!avc->AppendKey(key))
		{
			IOProcessor::Complete(avc);
			Init();
			avc->AppendKey(key);
		}
	}
	
	virtual bool Accept(const ByteString &key, const ByteString &)
	{
		// don't list system keys
		if (key.length >= 2 && key.buffer[0] == '@' && key.buffer[1] == '@')
			return true;

		if ((count == 0 || num < count) &&
			strncmp(keyHint.buffer, key.buffer, min(keyHint.length, key.length)) == 0)
		{
			AppendKey(key);
			num++;
			return true;
		}
		else
			return false;
	}
	
	virtual const ByteString* GetKeyHint()
	{
		return &keyHint;
	}
	
	virtual void OnComplete()
	{
		avc->complete = true;
		IOProcessor::Complete(avc);
		delete this;
	}
	
private:
	const ByteString		&keyHint;
	KeyspaceOp*				op;
	uint64_t				count;
	uint64_t				num;
	AsyncVisitorCallback*	avc;
};

class AsyncMultiDatabaseOp : public Callable, public MultiDatabaseOp
{
public:
	AsyncMultiDatabaseOp()
	{
		SetCallback(this);
	}
	
	void Execute()
	{
		Log_Trace();
		delete this;
	}
};

KeyspaceDB::KeyspaceDB()
{
	catchingUp = false;
}

bool KeyspaceDB::Init()
{
	Log_Trace();
	
	ReplicatedLog::Get()->SetReplicatedDB(this);
	
	table = database.GetTable("keyspace");
	
	catchupServer.Init(PaxosConfig::Get()->port + CATCHUP_PORT_OFFSET);
	catchupClient.Init(this, table);
	
	return true;
}

unsigned KeyspaceDB::GetNodeID()
{
	return ReplicatedLog::Get()->GetNodeID();
}

bool KeyspaceDB::Add(KeyspaceOp* op, bool submit)
{
	Log_Trace();

	bool ret;
	Transaction* transaction;
	
	if (catchingUp)
		return false;
	
	// don't allow writes for @@ keys
	if ((op->type == KeyspaceOp::SET || op->type == KeyspaceOp::TEST_AND_SET ||
		 op->type == KeyspaceOp::DELETE || op->type == KeyspaceOp::ADD)
		 && op->key.length > 2 && op->key.buffer[0] == '@' && op->key.buffer[1] == '@')
			return false;
	
	// reads are handled locally, they don't have to be added to the ReplicatedLog
	if (op->type == KeyspaceOp::DIRTY_GET || op->type == KeyspaceOp::GET)
	{
		// only handle GETs if I'm the master and it's safe to do so (I have NOPed)
		if (op->type == KeyspaceOp::GET &&
		   (!ReplicatedLog::Get()->IsMaster() || !ReplicatedLog::Get()->IsSafeDB()))
			return false;
		
		if ((transaction = ReplicatedLog::Get()->GetTransaction()) != NULL)
			if (!transaction->IsActive())
				transaction = NULL;
		
		op->value.Allocate(KEYSPACE_VAL_SIZE);
		ret = table->Get(transaction, op->key, op->value);

		op->client->OnComplete(op, ret);
		return true;
	}
	
	if (op->type == KeyspaceOp::LIST || op->type == KeyspaceOp::DIRTY_LIST)
	{
		if (op->type == KeyspaceOp::LIST &&
		   (!ReplicatedLog::Get()->IsMaster() || !ReplicatedLog::Get()->IsSafeDB()))
			return false;

		AsyncListVisitor *alv = new AsyncListVisitor(op->prefix, op, op->count);
		MultiDatabaseOp* mdbop = new AsyncMultiDatabaseOp();
		mdbop->Visit(table, *alv);
		dbReader.Add(mdbop);
		return true;
	}
	
	// only handle writes if I'm the master
	if (!ReplicatedLog::Get()->IsMaster())
		return false;
	
	ops.Append(op);
	
	if (submit && !ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster())
		Append();
	
	return true;
}

bool KeyspaceDB::Submit()
{
	// only handle writes if I'm the master
	if (!ReplicatedLog::Get()->IsMaster())
		return false;
	
	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster())
		Append();
	
	return true;
}

void KeyspaceDB::Execute(Transaction* transaction, bool ownAppend)
{
	bool		ret;
	int64_t		num;
	unsigned	nread;
	KeyspaceOp*	op;
	KeyspaceOp**it;
	
	ret = true;
	if (msg.type == KEYSPACE_GET)
		ASSERT_FAIL(); // GETs are not put in the ReplicatedLog
	else if (msg.type == KEYSPACE_SET)
		ret &= table->Set(transaction, msg.key, msg.value);
	else if (msg.type == KEYSPACE_TESTANDSET)
	{
		ret &= table->Get(transaction, msg.key, data);
		if (data == msg.test)
			ret &= table->Set(transaction, msg.key, msg.value);
		else
			ret = false;
	}
	else if (msg.type == KEYSPACE_ADD)
	{
		ret &= table->Get(transaction, msg.key, data); // read number
		
		if (ret)
		{
			num = strntoint64_t(data.buffer, data.length, &nread); // parse number
			if (nread == data.length)
			{
				num = num + msg.num;
				data.length = snprintf(data.buffer, data.size, "%" PRIi64 "", num); // print number
				ret &= table->Set(transaction, msg.key, data); // write number
			}
			else
				ret = false;
		}
	}
	else if (msg.type == KEYSPACE_DELETE)
		ret &= table->Delete(transaction, msg.key);
	else
		ASSERT_FAIL();
		
	if (ownAppend)
	{
		it = ops.Head();
		op = *it;
		if (op->type == KeyspaceOp::DIRTY_GET || op->type == KeyspaceOp::GET)
			ASSERT_FAIL();
		if (op->type == KeyspaceOp::ADD && ret)
		{
			// return new value to client
			op->value.Allocate(data.length);
			op->value.Set(data);

		}
		op->client->OnComplete(op, ret);
		ops.Remove(op);
	}
}

void KeyspaceDB::OnAppend(Transaction* transaction, uint64_t paxosID, ByteString value, bool ownAppend)
{
	unsigned numOps, nread;

	numOps = 0;
	while (true)
	{
		if (msg.Read(value, nread))
		{
			Execute(transaction, ownAppend);
			value.Advance(nread);
			numOps++;
			if (value.length == 0)
				break;
		}
		else
			break;
	}

	Log_Message("paxosID = %" PRIu64 ", numOps = %u", paxosID, numOps);
	
	if (ReplicatedLog::Get()->IsMaster() && ops.Length() > 0)
		Append();
}

void KeyspaceDB::Append()
{
	ByteString	bs;
	KeyspaceOp*	op;
	KeyspaceOp**it;

	data.length = 0;
	bs = data;

	for (it = ops.Head(); it != NULL; it = ops.Next(it))
	{
		op = *it;
		msg.FromKeyspaceOp(op);
		if (msg.Write(bs))
		{
			data.length += bs.length;
			bs.Advance(bs.length);
			
			if (bs.length <= 0)
				break;
		}
		else
			break;
	}
	
	ReplicatedLog::Get()->Append(data);
}

void KeyspaceDB::OnMasterLease(unsigned)
{
	if (!ReplicatedLog::Get()->IsAppending() && ReplicatedLog::Get()->IsMaster() && ops.Length() > 0)
		Append();
}

void KeyspaceDB::OnMasterLeaseExpired()
{
	Log_Trace();
}

void KeyspaceDB::OnDoCatchup(unsigned nodeID)
{
	Log_Trace();

	catchingUp = true;
	ReplicatedLog::Get()->Stop();
	if (ReplicatedLog::Get()->GetTransaction()->IsActive())
		ReplicatedLog::Get()->GetTransaction()->Commit();
	catchupClient.Start(nodeID);
}

void KeyspaceDB::OnCatchupComplete()
{
	Log_Trace();
	
	catchingUp = false;
	ReplicatedLog::Get()->Continue();
}

void KeyspaceDB::OnCatchupFailed()
{
	Log_Trace();

	catchingUp = false;
	ReplicatedLog::Get()->Continue();	
}
