#include <limits>
//#include <inttypes.h>

#include "KeyspaceClient.h"
#include "KeyspaceClientConn.h"
#include "KeyspaceCommand.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientReq.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientResp.h"
#include "Framework/PaxosLease/PLeaseConsts.h"


using namespace Keyspace;

/////////////////////////////////////////////////////////////////////
//
// Response implementation
//
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
//
// Result implementation
//
/////////////////////////////////////////////////////////////////////
//===================================================================
//
// ClientConn
//
//===================================================================

//===================================================================
//
// Command
//
//===================================================================
//===================================================================
//
// Client
//
//===================================================================
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

int Client::Init(int nodec, const char* nodev[], uint64_t timeout_)
{
	// validate args
	if (nodec <= 0 || nodev == NULL)
		return KEYSPACE_API_ERROR;

	
	timeout = timeout_;
	masterTimeout.SetDelay(2 * MAX_LEASE_TIME);
	globalTimeout.SetDelay(timeout);

	connectivityStatus = KEYSPACE_NOCONNECTION;
	timeoutStatus = KEYSPACE_SUCCESS;

	if (!IOProcessor::Init(nodec + 64))
		return KEYSPACE_API_ERROR;

	conns = new ClientConn*[nodec];
	
	for (int i = 0; i < nodec; i++)
	{
		Endpoint endpoint;
		
		endpoint.Set(nodev[i], true);
		conns[i] = new ClientConn(*this, i, endpoint, timeout);
	}
	
	numConns = nodec;
	master = -1;
	masterTime = 0;
	cmdID = 1;
	masterQuery = false;
	distributeDirty = false;
	autoFailover = true;
	currentConn = 0;
	
	return KEYSPACE_SUCCESS;
}

uint64_t Client::SetTimeout(uint64_t timeout_)
{
	uint64_t	prev;
	
	prev = timeout;
	timeout = timeout_;
		
	return prev;
}

uint64_t Client::GetTimeout()
{
	return timeout;
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
		cmd = CreateCommand(KEYSPACECLIENT_DIRTY_COUNT,	SIZE(args), args);
		dirtyCommands.Append(cmd);
	}
	else
	{
		cmd = CreateCommand(KEYSPACECLIENT_COUNT, SIZE(args), args);
		safeCommands.Append(cmd);
	}
	
	result->Close();
	result->AppendCommand(cmd);
	
	EventLoop();
	status = result->TransportStatus();
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

	args[0] = key;

	if (dirty)
	{
		cmd = CreateCommand(KEYSPACECLIENT_DIRTY_GET, 1, args);
		dirtyCommands.Append(cmd);
	}
	else
	{
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
	return result->TransportStatus();
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
		if (values)
			cmd = CreateCommand(KEYSPACECLIENT_DIRTY_LISTP, SIZE(args), args);
		else
			cmd = CreateCommand(KEYSPACECLIENT_DIRTY_LIST, SIZE(args), args);

		dirtyCommands.Append(cmd);
	}
	else
	{
		if (values)
			cmd = CreateCommand(KEYSPACECLIENT_LISTP, SIZE(args), args);
		else
			cmd = CreateCommand(KEYSPACECLIENT_LIST, SIZE(args), args);
		
		safeCommands.Append(cmd);
	}
	
	result->Close();
	result->AppendCommand(cmd);
	
	EventLoop();
	return result->TransportStatus();	

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

int Client::Set(const ByteString& key, const ByteString& value, bool submit)
{
	Command*	cmd;
	ByteString	args[2];
	int			status;
	
	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;	
	
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
	status = result->TransportStatus();
	
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
	status = result->TransportStatus();
	
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
	status = result->TransportStatus();
	
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
	status = result->TransportStatus();
	
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
	status = result->TransportStatus();
	
	return status;
}

int Client::Prune(const ByteString &prefix, bool submit)
{
	int			status;
	Command*	cmd;
	ByteString	args[1];
	
	if (result->isBatched && submit)
		return KEYSPACE_API_ERROR;

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
	status = result->TransportStatus();
	
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

	result->Begin();
}

bool Client::IsDone()
{
	if (masterQuery && master != -1)
	{
		result->transportStatus = KEYSPACE_SUCCESS;
		return true;
	}
	
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

uint64_t Client::GetNextID()
{
	return cmdID++;
}

Command* Client::CreateCommand(char type, int msgc, ByteString *msgv)
{
	Command*		cmd;
	int				len;
	char			tmp[20];
	char			idbuf[20];
	int				idlen;
	int				keyOffs;
	int				keyLen;
	

	cmd = new Command;
	cmd->cmdID = GetNextID() * 10;
	cmd->type = type;
	cmd->nodeID = -1;
	
	if (cmd->type == KEYSPACECLIENT_GET_MASTER)
		cmd->cmdID += KEYSPACE_MOD_GETMASTER;
	else if (cmd->IsDirty())
		cmd->cmdID += KEYSPACE_MOD_DIRTY_COMMAND;
	else
		cmd->cmdID += KEYSPACE_MOD_SAFE_COMMAND;
	
	len = 2; // command + colon
	idlen = snwritef(idbuf, sizeof(idbuf), "%U", cmd->cmdID);
	len += idlen;
	for (int i = 0; msgv && i < msgc; i++)
		len += 1 + NumLen(msgv[i].length) + 1 + msgv[i].length;
	
	len = snwritef(tmp, sizeof(tmp), "%d:", len);
	cmd->msg.Append(tmp, len);
	
	cmd->msg.Append(&type, 1);
	cmd->msg.Append(":", 1);
	cmd->msg.Append(idbuf, idlen);
	
	for (int i = 0; msgv && i < msgc; i++)
	{
		len = snwritef(tmp, sizeof(tmp), ":%d:", msgv[i].length);
		cmd->msg.Append(tmp, len);

		// HACK!
		if (i == 0)
		{
			keyOffs = cmd->msg.length;
			keyLen = msgv[i].length;
		}
		
		cmd->msg.Append(msgv[i].buffer, msgv[i].length);
	}
	
	cmd->key.buffer = cmd->msg.buffer + keyOffs;
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
			Log_Trace("node %d became the master", nodeID);
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
		Log_Trace("node %d lost its mastership", nodeID);
		master = -1;
		connectivityStatus = KEYSPACE_NOMASTER;
		
		for (it = result->commands.Head(); it != NULL; it = result->commands.Next(it))
		{
			cmd = *it;
			if (cmd->status == KEYSPACE_NOSERVICE && cmd->nodeID == nodeID)
			{
				cmd->nodeID = -1;
				cmd->ClearResponse();
				if (!cmd->IsDirty())
					safeCommands.Append(cmd);
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
