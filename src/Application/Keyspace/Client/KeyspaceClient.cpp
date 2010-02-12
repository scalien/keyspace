#include <limits>
//#include <inttypes.h>

#include "KeyspaceClient.h"
#include "KeyspaceClientConn.h"
#include "KeyspaceCommand.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientReq.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientResp.h"
#include "Framework/PaxosLease/PLeaseConsts.h"

#define VALIDATE_KEY_LEN(k) 	if (k.length > KEYSPACE_KEY_SIZE) return KEYSPACE_API_ERROR
#define VALIDATE_VAL_LEN(v) 	if (v.length > KEYSPACE_VAL_SIZE) return KEYSPACE_API_ERROR

#define VALIDATE_DIRTY() if (safeCommands.Length() > 0) return KEYSPACE_API_ERROR
#define VALIDATE_SAFE() if (dirtyCommands.Length() > 0) return KEYSPACE_API_ERROR

using namespace Keyspace;

Client::Client() :
onGlobalTimeout(this, &Client::OnGlobalTimeout),
globalTimeout(&onGlobalTimeout),
onMasterTimeout(this, &Client::OnMasterTimeout),
masterTimeout(&onMasterTimeout)
{
	numConns = 0;
	conns = NULL;
	result = new Result;
}

Client::~Client()
{
	delete result;
	for (int i = 0; i < numConns; i++)
	{
		delete conns[i];
		conns[i] = NULL;
	}
	delete[] conns;
	IOProcessor::Shutdown();
}

int Client::Init(int nodec, const char* nodev[])
{
	// validate args
	if (nodec <= 0 || nodev == NULL)
		return KEYSPACE_API_ERROR;

	masterTimeout.SetDelay(3 * MAX_LEASE_TIME);
	globalTimeout.SetDelay(KEYSPACE_DEFAULT_TIMEOUT);

	connectivityStatus = KEYSPACE_NOCONNECTION;
	timeoutStatus = KEYSPACE_SUCCESS;

	if (!IOProcessor::Init(nodec + 64))
		return KEYSPACE_API_ERROR;

	conns = new ClientConn*[nodec];
	
	for (int i = 0; i < nodec; i++)
	{
		Endpoint endpoint;
		
		endpoint.Set(nodev[i], true);
		conns[i] = new ClientConn(*this, i, endpoint);
	}
	
	numConns = nodec;
	master = -1;
	masterTime = 0;
	cmdID = 1;
	masterCmdID = 1;
	masterQuery = false;
	distributeDirty = false;
	autoFailover = true;
	currentConn = 0;
	
	return KEYSPACE_SUCCESS;
}

void Client::SetGlobalTimeout(uint64_t timeout)
{
	globalTimeout.SetDelay(timeout);
}

void Client::SetMasterTimeout(uint64_t timeout)
{
	masterTimeout.SetDelay(timeout);
}

uint64_t Client::GetGlobalTimeout()
{
	return globalTimeout.GetDelay();
}

uint64_t Client::GetMasterTimeout()
{
	return masterTimeout.GetDelay();
}

int Client::GetMaster()
{
	if (!conns)
		return KEYSPACE_API_ERROR;

	masterQuery = true;
	
	EventLoop();
	if (connectivityStatus < 0)
		return connectivityStatus;
		
	return master;
}

void Client::DistributeDirty(bool dd)
{
	distributeDirty = dd;
}

void Client::SetAutoFailover(bool fo)
{
	autoFailover = fo;
}

int Client::Get(const ByteString &key, ByteString &value, bool dirty)
{
	int			ret;
	ByteString	tmp;

	ret = Get(key, dirty);
	if (ret < 0)
		return ret;
	
	ret = result->Value(tmp);
	value.Set(tmp);
	
	return ret;
}

int	Client::DirtyGet(const ByteString &key, ByteString &value)
{
	return Get(key, value, true);
}

int Client::Count(uint64_t &res, const ByteString &prefix,
const ByteString &startKey,
uint64_t count, bool next, bool forward)
{
	return Count(res, prefix, startKey, count, next, forward, false);
}

int Client::DirtyCount(uint64_t &res, const ByteString &prefix,
const ByteString &startKey,
uint64_t count, bool next, bool forward)
{
	return Count(res, prefix, startKey, count, next, forward, true);
}

int Client::Count(uint64_t &res, const ByteString &prefix,
const ByteString &startKey,
uint64_t count = 0, bool next = false, bool forward = false, bool dirty = false)
{
	Command*		cmd;
	ByteString		args[5];
	DynArray<32>	countString;
	DynArray<10>	nextString;
	DynArray<10>	backString;
	ByteString		sk;
	int				status;
	unsigned		nread;
	ByteString		value;
	
	if (result->isBatched)
		return KEYSPACE_API_ERROR;

	VALIDATE_KEY_LEN(prefix);
	
	countString.Writef("%U", count);
	if (next)
		nextString.Append("1", 1);
	else
		nextString.Append("0", 1);

	if (forward)
		backString.Append("f", 1);
	else
		backString.Append("b", 1);

	if (prefix.length > 0 && startKey.length >= prefix.length)
	{
		if (memcmp(prefix.buffer, startKey.buffer,
			MIN(prefix.length, startKey.length)) == 0)
		{
			sk.buffer = startKey.buffer + prefix.length;
			sk.length = startKey.length - prefix.length;
			sk.size = startKey.size - prefix.length;
		}
	}
	else
		sk = startKey; // TODO: this seems inconsistent

	args[0] = prefix;
	args[1] = sk;
	args[2] = countString;
	args[3] = nextString;
	args[4] = backString;
	
	if (dirty)
	{
		VALIDATE_DIRTY();
		cmd = CreateCommand(KEYSPACECLIENT_DIRTY_COUNT,	SIZE(args), args);
		dirtyCommands.Append(cmd);
	}
	else
	{
		VALIDATE_SAFE();
		cmd = CreateCommand(KEYSPACECLIENT_COUNT, SIZE(args), args);
		safeCommands.Append(cmd);
	}
	
	result->Close();
	result->AppendCommand(cmd);
	
	EventLoop();
	status = result->CommandStatus();
	if (status != KEYSPACE_SUCCESS)
		return status;
	
	// TODO check conversion
	status = result->Value(value);
	if (status == KEYSPACE_SUCCESS)
		res = strntoint64(value.buffer, value.length, &nread);

	return status;
}

int Client::Get(const ByteString &key, bool dirty, bool submit)
{
	Command*	cmd;
	ByteString	args[1];

	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;

	VALIDATE_KEY_LEN(key);

	args[0] = key;

	if (dirty)
	{
		VALIDATE_DIRTY();
		cmd = CreateCommand(KEYSPACECLIENT_DIRTY_GET, 1, args);
		dirtyCommands.Append(cmd);
	}
	else
	{
		VALIDATE_SAFE();
		cmd = CreateCommand(KEYSPACECLIENT_GET, 1, args);
		safeCommands.Append(cmd);
	}

	if (!submit)
	{
		result->AppendCommand(cmd);
		return KEYSPACE_SUCCESS;
	}
	
	result->Close();
	result->AppendCommand(cmd);
	
	EventLoop();	
	return result->CommandStatus();
}

int Client::DirtyGet(const ByteString &key, bool submit)
{
	return Get(key, true, submit);
}

int	Client::ListKeys(const ByteString &prefix,
const ByteString &startKey, uint64_t count, bool next, bool forward)
{
	return ListKeyValues(prefix, startKey, count, next, forward, false, false);
}

int	Client::DirtyListKeys(const ByteString &prefix,
const ByteString &startKey, uint64_t count, bool next, bool forward)
{
	return ListKeyValues(prefix, startKey, count, next, forward, true, false);
}

int Client::ListKeyValues(const ByteString &prefix,
const ByteString &startKey, uint64_t count, bool next, bool forward)
{
	return ListKeyValues(prefix, startKey, count, next, forward, false, true);
}

int Client::DirtyListKeyValues(const ByteString &prefix,
const ByteString &startKey, uint64_t count, bool next, bool forward)
{
	return ListKeyValues(prefix, startKey, count, next, forward, true, true);
}

int Client::ListKeyValues(const ByteString &prefix,
const ByteString &startKey, uint64_t count,
bool next, bool forward, bool dirty, bool values)
{
	Command*		cmd;
	ByteString		args[5];
	DynArray<32>	countString;
	DynArray<10>	nextString;
	DynArray<10>	backString;
	ByteString		sk;

	if (result->isBatched)
		return KEYSPACE_API_ERROR;
	
	VALIDATE_KEY_LEN(prefix);

	countString.Writef("%U", count);
	if (next)
		nextString.Append("1", 1);
	else
		nextString.Append("0", 1);

	if (forward)
		backString.Append("f", 1);
	else
		backString.Append("b", 1);

	if (prefix.length > 0 && startKey.length >= prefix.length)
	{
		if (memcmp(prefix.buffer, startKey.buffer,
			MIN(prefix.length, startKey.length)) == 0)
		{
			sk.buffer = startKey.buffer + prefix.length;
			sk.length = startKey.length - prefix.length;
			sk.size = startKey.size - prefix.length;
		}
	}
	else
		sk = startKey; // TODO: this seems inconsistent

	args[0] = prefix;
	args[1] = sk;
	args[2] = countString;
	args[3] = nextString;
	args[4] = backString;
	
	if (dirty)
	{
		VALIDATE_DIRTY();
		
		if (values)
			cmd = CreateCommand(KEYSPACECLIENT_DIRTY_LISTP, SIZE(args), args);
		else
			cmd = CreateCommand(KEYSPACECLIENT_DIRTY_LIST, SIZE(args), args);

		dirtyCommands.Append(cmd);
	}
	else
	{
		VALIDATE_SAFE();
		
		if (values)
			cmd = CreateCommand(KEYSPACECLIENT_LISTP, SIZE(args), args);
		else
			cmd = CreateCommand(KEYSPACECLIENT_LIST, SIZE(args), args);
		
		safeCommands.Append(cmd);
	}
	
	result->Close();
	result->AppendCommand(cmd);
	
	EventLoop();
	return result->CommandStatus();	

}


Result* Client::GetResult()
{
	Result*		tmp;
	
	if (conns == NULL)
		return NULL;

	tmp = result;
	result = new Result();
	return tmp;
}

int Client::TransportStatus()
{
	return result->TransportStatus();
}

int Client::ConnectivityStatus()
{
	return connectivityStatus;
}

int Client::TimeoutStatus()
{
	return timeoutStatus;
}

int Client::CommandStatus()
{
	return result->CommandStatus();
}

int Client::Set(const ByteString& key, const ByteString& value, bool submit)
{
	Command*	cmd;
	ByteString	args[2];
	int			status;
	
	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;	
	
	VALIDATE_KEY_LEN(key);
	VALIDATE_VAL_LEN(value);
	VALIDATE_SAFE();

	args[0] = key;
	args[1] = value;
	
	cmd = CreateCommand(KEYSPACECLIENT_SET, 2, args);
	safeCommands.Append(cmd);
	
	if (!submit)
	{
		result->AppendCommand(cmd);
		return KEYSPACE_SUCCESS;
	}
	
	result->Close();
	result->AppendCommand(cmd);

	EventLoop();
	status = result->CommandStatus();
	
	return status;
}

int Client::TestAndSet(const ByteString &key,
const ByteString &test, const ByteString &value, bool submit)
{
	Command*	cmd;
	ByteString	args[3];
	int			status;
	
	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;

	VALIDATE_KEY_LEN(key);
	VALIDATE_VAL_LEN(test);
	VALIDATE_VAL_LEN(value);
	VALIDATE_SAFE();
	
	args[0] = key;
	args[1] = test;
	args[2] = value;
	
	cmd = CreateCommand(KEYSPACECLIENT_TEST_AND_SET, 3, args);
	safeCommands.Append(cmd);
	
	if (!submit)
	{
		result->AppendCommand(cmd);
		return KEYSPACE_SUCCESS;
	}
	
	result->Close();
	result->AppendCommand(cmd);

	EventLoop();
	status = result->CommandStatus();
	
	return status;
}

int Client::Add(const ByteString &key, int64_t num, int64_t &res, bool submit)
{
	Command*	cmd;
	ByteString	args[2];
	DynArray<32> numString;
	unsigned	nread;
	int			status;
	ByteString	value;

	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;

	VALIDATE_KEY_LEN(key);
	VALIDATE_SAFE();

	numString.Writef("%U", num);
	
	args[0] = key;
	args[1] = numString;
	
	cmd = CreateCommand(KEYSPACECLIENT_ADD, 2, args);
	safeCommands.Append(cmd);
	
	if (!submit)
	{
		result->AppendCommand(cmd);
		return KEYSPACE_SUCCESS;
	}
	
	result->Close();
	result->AppendCommand(cmd);
	
	EventLoop();
	status = result->CommandStatus();
	
	if (status != KEYSPACE_SUCCESS)
		return status;
	
	status = result->Value(value);
	// TODO: check conversion
	if (status == KEYSPACE_SUCCESS)
		res = strntoint64(value.buffer, value.length, &nread);

	return status;
}

int Client::Delete(const ByteString &key, bool submit, bool remove)
{
	int			status;
	char		c;
	Command*	cmd;
	ByteString	args[1];

	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;

	VALIDATE_KEY_LEN(key);
	VALIDATE_SAFE();

	args[0] = key;
	
	if (remove)
		c = KEYSPACECLIENT_REMOVE;
	else
		c = KEYSPACECLIENT_DELETE;
		
	cmd = CreateCommand(c, SIZE(args), args);
	safeCommands.Append(cmd);
	
	if (!submit)
	{
		result->AppendCommand(cmd);
		return KEYSPACE_SUCCESS;
	}
	
	result->Close();
	result->AppendCommand(cmd);

	EventLoop();
	status = result->CommandStatus();
	
	return status;
}

int Client::Remove(const ByteString &key, bool submit)
{
	return Delete(key, submit, true);
}

int Client::Rename(const ByteString &from, const ByteString &to, bool submit)
{
	int			status;
	Command*	cmd;
	ByteString	args[2];

	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;

	VALIDATE_KEY_LEN(from);
	VALIDATE_KEY_LEN(to);
	VALIDATE_SAFE();

	args[0] = from;
	args[1] = to;
	
	cmd = CreateCommand(KEYSPACECLIENT_RENAME, SIZE(args), args);
	safeCommands.Append(cmd);
	
	if (!submit)
	{
		result->AppendCommand(cmd);
		return KEYSPACE_SUCCESS;
	}
		
	result->Close();
	result->AppendCommand(cmd);
	
	EventLoop();
	status = result->CommandStatus();
	
	return status;
}

int Client::Prune(const ByteString &prefix, bool submit)
{
	int			status;
	Command*	cmd;
	ByteString	args[1];
	
	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;

	VALIDATE_KEY_LEN(prefix);
	VALIDATE_SAFE();

	args[0] = prefix;
	
	cmd = CreateCommand(KEYSPACECLIENT_PRUNE, SIZE(args), args);
	safeCommands.Append(cmd);
	
	if (!submit)
	{
		result->AppendCommand(cmd);
		return KEYSPACE_SUCCESS;
	}
	
	result->Close();
	result->AppendCommand(cmd);

	EventLoop();
	status = result->CommandStatus();
	
	return status;
}

int Client::Begin()
{
	if (!conns)
		return KEYSPACE_API_ERROR;

	if (result->isBatched)
		return KEYSPACE_API_ERROR;

	result->Close();
	result->isBatched = true;

	return KEYSPACE_SUCCESS;
}

int Client::Submit()
{
	if (!conns)
		return KEYSPACE_API_ERROR;
	
	EventLoop();
	result->isBatched = false;
	
	return result->TransportStatus();
}

int Client::Cancel()
{
	if (!conns)
		return KEYSPACE_API_ERROR;
	
	safeCommands.Clear();
	dirtyCommands.Clear();
	
	result->isBatched = false;
	result->Close();
	
	return KEYSPACE_SUCCESS;
}

void Client::EventLoop()
{
	if (!conns)
	{
		// TODO: HACK:
		result->transportStatus = KEYSPACE_API_ERROR;
		return;
	}
	
	result->InitCommandMap();
	
	EventLoop::UpdateTime();
	EventLoop::Reset(&globalTimeout);
	EventLoop::Reset(&masterTimeout);
	timeoutStatus = KEYSPACE_SUCCESS;
	if (master != -1)
		SendCommand(conns[master], safeCommands);
	
	SendDirtyCommands();

	while (!IsDone())
	{
		if (!EventLoop::RunOnce())
			break;
	}

	safeCommands.Clear();
	dirtyCommands.Clear();

	result->FreeCommandMap();
	result->connectivityStatus = connectivityStatus;
	result->timeoutStatus = timeoutStatus;

	result->Begin();
}

bool Client::IsDone()
{
	if (masterQuery && master != -1)
	{
		result->transportStatus = KEYSPACE_SUCCESS;
		return true;
	}
	
	assert(result->commands.Length() >= result->numCompleted);
	
	if (result->commands.Length() == result->numCompleted)
	{
		result->transportStatus = KEYSPACE_SUCCESS;
		return true;
	}
	
	if (timeoutStatus != KEYSPACE_SUCCESS)
		return true;
	
	return false;
}

void Client::SendCommand(ClientConn* conn, CommandList& commands)
{
//	Log_Message("Sending command");

	Command**	it;
	
	it = commands.Head();
	if (it != NULL)
	{
		conn->Send(**it);
		commands.Remove(it);
	}
}

void Client::SendDirtyCommands()
{
	int	i;
	
	for (i = 0; i < numConns; i++)
	{
		if (conns[i]->GetState() == ClientConn::CONNECTED)
			SendCommand(conns[i], dirtyCommands);
	}
}

uint64_t Client::NextMasterCommandID()
{
	masterCmdID++;
	return (masterCmdID * 10 + KEYSPACE_MOD_GETMASTER);
}

uint64_t Client::NextCommandID()
{
	cmdID++;
	return (cmdID * 10 + KEYSPACE_MOD_COMMAND);
}

Command* Client::CreateCommand(char type, int msgc, ByteString *msgv)
{
	Command*		cmd;
	int				len;
	char			tmp[20];
	int				keyOffs;
	int				keyLen;
	

	cmd = new Command;
	cmd->type = type;
	cmd->nodeID = -1;
	
	if (cmd->type == KEYSPACECLIENT_GET_MASTER)
		cmd->cmdID = NextMasterCommandID();
	else
		cmd->cmdID = NextCommandID();
	
	// serialize args
	for (int i = 0; msgv && i < msgc; i++)
	{
		len = snwritef(tmp, sizeof(tmp), ":%d:", msgv[i].length);
		cmd->args.Append(tmp, len);

		// HACK!
		if (i == 0)
		{
			keyOffs = cmd->args.length;
			keyLen = msgv[i].length;
		}
		
		cmd->args.Append(msgv[i].buffer, msgv[i].length);
	}
	
	cmd->key.buffer = cmd->args.buffer + keyOffs;
	cmd->key.length = keyLen;
	cmd->key.size = keyLen;
	
	return cmd;
}

void Client::SetMaster(int master_, int nodeID)
{
	Log_Trace("known master: %d, set master: %d, nodeID: %d", master, master_, nodeID);

	Command**	it;
	Command*	cmd;
	
	if (master_ == nodeID)
	{
		if (master != master_)
		{
			// node became the master
			Log_Message("Node %d is the master", nodeID);
			master = master_;
			connectivityStatus = KEYSPACE_SUCCESS;
			SendCommand(conns[master], safeCommands);
		}
		// else node is still the master
		
		EventLoop::Reset(&masterTimeout);
	}
	else if (master_ < 0 && master == nodeID)
	{
		// node lost its mastership
		Log_Message("Node %d lost its mastership", nodeID);
		master = -1;
		connectivityStatus = KEYSPACE_NOMASTER;
		
		if (!IsSafe())
			return;
		
		for (it = result->commands.Head(); it != NULL; it = result->commands.Next(it))
		{
			cmd = *it;
			cmd->cmdID = NextCommandID();
			Log_Trace("new command ID: %" PRIu64 "", cmd->cmdID);
			if (cmd->status == KEYSPACE_NOSERVICE && cmd->nodeID == nodeID)
			{
				Log_Trace("appending to safecommands");
				cmd->nodeID = -1;
				cmd->ClearResponse();
				if (!cmd->IsDirty())
					safeCommands.Append(cmd);
			}
			else
			{
				Log_Trace("NOT appending to safecommands: %d %d",
				cmd->status, cmd->nodeID);	
			}
		}

		// set master timeout
	}
}

void Client::OnGlobalTimeout()
{
	Log_Trace();
	timeoutStatus = KEYSPACE_GLOBAL_TIMEOUT;
}

void Client::OnMasterTimeout()
{
	Log_Trace();
	timeoutStatus = KEYSPACE_MASTER_TIMEOUT;
}

bool Client::IsSafe()
{
	Command**	it;
	Command*	cmd;

	it = result->commands.Head();
	if (!it)
		return false;
	cmd = *it;
	if (cmd->IsDirty())
		return false; 

	return true;
}
