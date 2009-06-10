#include "KeyspaceClient.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientReq.h"
#include "Application/Keyspace/Protocol/Keyspace/KeyspaceClientResp.h"

using namespace Keyspace;

/////////////////////////////////////////////////////////////////////
//
// Response implementation
//
/////////////////////////////////////////////////////////////////////
bool Response::Read(const ByteString& data_)
{
	bool		ret;
	char		cmd;
	uint64_t	cmdID;
	ByteString	tmp;

	status = KEYSPACE_ERROR;
	data = data_;
	pos = data.buffer;
	separator = ':';

	msg.Clear();
	key.Clear();
	value.Clear();
	
	ret = ReadChar(cmd);
	ret = ret && ReadSeparator();
	ret = ret && ReadUint64(cmdID);
	if (!ret)
		return false;
	
	id = cmdID;
	
	if (cmd == KEYSPACECLIENT_NOT_MASTER)
	{
		status = KEYSPACE_NOTMASTER;
		return ValidateLength();
	}
	
	if (cmd == KEYSPACECLIENT_FAILED)
	{
		status = KEYSPACE_FAILED;
		return ValidateLength();
	}
	
	if (cmd == KEYSPACECLIENT_LIST_END)
	{
		status = KEYSPACE_OK;
		return ValidateLength();
	}
	
	if (cmd == KEYSPACECLIENT_OK)
	{
		status = KEYSPACE_OK;
		ret = ReadMessage(tmp);
		if (ret)
			value.Append(tmp.buffer, tmp.length);

		return ValidateLength();
	}
	
	if (cmd == KEYSPACECLIENT_LIST_ITEM)
	{
		status = KEYSPACE_OK;
		ret = ReadMessage(tmp);
		if (ret)
			key.Append(tmp.buffer, tmp.length);
		ret = ret && ValidateLength();
		return ret;
	}
	
	if (cmd == KEYSPACECLIENT_LISTP_ITEM)
	{
		status = KEYSPACE_OK;
		ret = ReadMessage(tmp);
		if (ret)
			key.Append(tmp.buffer, tmp.length);
		ret = ret && ReadMessage(tmp);
		if (ret)
			value.Append(tmp.buffer, tmp.length);
		
		ret = ret && ValidateLength();
		return ret;
	}
	
	return false;
}

bool Response::CheckOverflow()
{
	if ((pos - data.buffer) >= (int) data.length || pos < data.buffer)
		return false;
	return true;
}

bool Response::ReadUint64(uint64_t& num)
{
	unsigned	nread;
	
	if (!CheckOverflow())
		return false;

	num = strntouint64(pos, data.length - (pos - data.buffer), &nread);
	if (nread < 1)
		return false;
	
	pos += nread;
	return true;
}

bool Response::ReadChar(char& c)
{
	if (!CheckOverflow())
		return false;

	c = *pos; 
	pos++;
	
	return true;
}

bool Response::ReadSeparator()
{
	if (!CheckOverflow())
		return false;
	
	if (*pos != separator) 
		return false; 
	
	pos++;
	return true;
}

bool Response::ReadMessage(ByteString& bs)
{
	bool		ret;
	uint64_t	num;
	
	ret = true;
	ret = ret && ReadSeparator();
	ret = ret && ReadUint64(num);
	ret = ret && ReadSeparator();
	ret = ret && ReadData(bs, num);
	
	return ret;
}

bool Response::ReadData(ByteString& bs, uint64_t length)
{
	if (pos - length > data.buffer + data.length)
		return false;
	
	bs.buffer = pos;
	bs.length = length;

	pos += length;
	
	return true;
}

bool Response::ValidateLength()
{
	if ((pos - data.buffer) != (int) data.length) 
		return false;
	
	return true;
}

/////////////////////////////////////////////////////////////////////
//
// Result implementation
//
/////////////////////////////////////////////////////////////////////


void Result::Close()
{
	Response**	it;
	
	while ((it = responses.Head()) != NULL)
	{
		responses.Remove(it);
		delete *it;
	}
}

Result* Result::Next(int &status)
{
	Response**	it;

	it = responses.Head();
	if (!it)
	{
		status = KEYSPACE_OK;
		return NULL;
	}

	delete *it;
	responses.Remove(it);

	if (!responses.Head())
	{
		status = KEYSPACE_OK;
		return NULL;
	}
	
	return this;
}

const ByteString& Result::Key()
{
	Response**	it;
	
	it = responses.Head();
	if (it)
		return (*it)->key;
		
	return empty;
}

const ByteString& Result::Value()
{
	Response**	it;
	
	it = responses.Head();
	if (it)
		return (*it)->value;
		
	return empty;
}

int Result::Status()
{
	Response**	it;
	
	it = responses.Head();
	if (it)
		return (*it)->status;
		
	return KEYSPACE_ERROR;
}

void Result::AppendResponse(Response* resp_)
{
	responses.Append(resp_);
}

//===================================================================
//
// ClientConn
//
//===================================================================
ClientConn::ClientConn(Client &client, int nodeID_, const Endpoint &endpoint_) :
client(client),
endpoint(endpoint_)
{
	nodeID = nodeID_;
	disconnectTime = 0;
	getMasterTime = 0;
	getMasterPending = false;
}

void ClientConn::Send(Command &cmd)
{
	Log_Trace("nodeID = %d", nodeID);
	cmd.nodeID = nodeID;
	
	if (cmd.submit)
	{
		Write(cmd.msg.buffer, cmd.msg.length, false);
		Write("1:*", 3);
	}
	else
	{
		Write(cmd.msg.buffer, cmd.msg.length);
	}
}

bool ClientConn::ProcessMessage(Response* resp)
{
	Command**	it;
	unsigned	nread;
	
	for (it = client.sentCommands.Head(); it != NULL; /* advanced in body */)
	{
		Command* cmd = *it;
		
		if (cmd->cmdID == resp->id)
		{
			cmd->status = resp->status;
			
			// GET_MASTER is a special case, because it is only used internally
			if (cmd->type == KEYSPACECLIENT_GET_MASTER)
			{
				getMasterPending = false;
				if (resp->status == KEYSPACE_OK)
					client.SetMaster((int) strntoint64(resp->value.buffer, resp->value.length, &nread));
				else
					client.SetMaster(-1);
					
				if (nread != resp->value.length)
					resp->status = KEYSPACE_ERROR;
				
				delete cmd;
				client.sentCommands.Remove(it);
				return false;
			}
			
			// with LIST style commands there can be more than one response with
			// the same id
			if (cmd->type == KEYSPACECLIENT_LIST ||
				cmd->type == KEYSPACECLIENT_LISTP ||
				cmd->type == KEYSPACECLIENT_DIRTY_LIST ||
				cmd->type == KEYSPACECLIENT_DIRTY_LISTP)
			{
				// key.length == 0 means end of the list response
				if (resp->key.length == 0)
				{
					delete cmd;
					client.sentCommands.Remove(it);
				}
				else
					client.result.AppendResponse(resp);
			}
			else
			{
				delete cmd;
				client.sentCommands.Remove(it);
				client.result.AppendResponse(resp);
			}
			
			return true;
		}
		else
			it = client.sentCommands.Next(it);
	}
	
	return true;
}

void ClientConn::DeleteCommands()
{
	Command**	it;

	for (it = client.sentCommands.Head(); it != NULL; /* advanced in body */)
	{
		Command*	cmd;

		cmd = *it;
		if (cmd->nodeID == nodeID)
		{
			it = client.sentCommands.Remove(it);
			client.result.Close();
		}
		else
			it = client.sentCommands.Next(it);
	}
}

void ClientConn::GetMaster()
{
	Command* cmd;
	
	cmd = client.CreateCommand(KEYSPACECLIENT_GET_MASTER, false, 0, NULL);
	client.sentCommands.Append(cmd);
	Send(*cmd);
	getMasterPending = true;
	getMasterTime = Now();
}

Endpoint& ClientConn::GetEndpoint()
{
	return endpoint;
}

void ClientConn::OnMessageRead(const ByteString& msg)
{
	Response	*resp;
	
	resp = new Response;
	if (resp->Read(msg))
	{
		if (!ProcessMessage(resp))
			delete resp;
		client.StateFunc();
	}
	else
		delete resp;
}

void ClientConn::OnWrite()
{
	TCPConn<>::OnWrite();
	if (!tcpwrite.active)
		client.StateFunc();
}

void ClientConn::OnClose()
{
	Close();
	client.StateFunc();
	DeleteCommands();
	disconnectTime = Now();
}

void ClientConn::OnConnect()
{
	TCPConn<>::OnConnect();
	AsyncRead();
	client.StateFunc();
}

void ClientConn::OnConnectTimeout()
{
	OnClose();
}

//===================================================================
//
// Command
//
//===================================================================
Command::Command()
{
	type = 0;
	nodeID = -1;
	status = 0;
	cmdID = 0;
	submit = false;
}

//===================================================================
//
// Client
//
//===================================================================
Client::~Client()
{
	for (int i = 0; i < numConns; i++)
	{
		delete conns[i];
	}
	delete[] conns;
	IOProcessor::Shutdown();
}

int Client::Init(int nodec, const char* nodev[], uint64_t timeout_)
{
	timeout = timeout_;
	reconnectTimeout = timeout;
	IOProcessor::Init(nodec + 64);
	conns = new ClientConn*[nodec];
	
	for (int i = 0; i < nodec; i++)
	{
		Endpoint endpoint;
		
		endpoint.Set(nodev[i]);
		conns[i] = new ClientConn(*this, i, endpoint);
	}
	numConns = nodec;
	master = -1;
	masterTime = 0;
	cmdID = 0;
	distributeDirty = false;
	
	return KEYSPACECLIENT_OK;
}

int Client::GetMaster()
{
	return master;
}

int Client::Get(const ByteString &key, ByteString &value, bool dirty)
{
	int		ret;

	ret = Get(key, dirty);
	if (ret < 0)
		return ret;
	
	if (value.size < result.Value().length)
		return result.Value().length;
	
	value.Set(result.Value());
	return KEYSPACE_OK;
}

void Client::DistributeDirty(bool dd)
{
	distributeDirty = dd;
}

int	Client::DirtyGet(const ByteString &key, ByteString &value)
{
	return Get(key, value, true);
}

int Client::Get(const ByteString &key, bool dirty)
{
	Command*	cmd;
	ByteString	args[1];

	args[0] = key;

	if (dirty)
	{
		cmd = CreateCommand(KEYSPACECLIENT_DIRTY_GET, false, 1, args);
		dirtyCommands.Add(cmd);
	}
	else
	{
		cmd = CreateCommand(KEYSPACECLIENT_GET, false, 1, args);
		safeCommands.Add(cmd);
	}

	result.Close();
	
	EventLoop();	
	return result.Status();
}

int Client::DirtyGet(const ByteString &key)
{
	return Get(key, true);
}

int	Client::List(const ByteString &prefix, uint64_t count, bool dirty)
{
	Command*	cmd;
	ByteString	args[2];
	DynArray<32> countString;
	
	countString.Writef("%U", count);
	
	args[0] = prefix;
	args[1] = countString;
	
	if (dirty)
	{
		cmd = CreateCommand(KEYSPACECLIENT_DIRTY_LIST, false, 2, args);
		dirtyCommands.Add(cmd);
	}
	else
	{
		cmd = CreateCommand(KEYSPACECLIENT_LIST, false, 2, args);
		safeCommands.Add(cmd);
	}
	
	result.Close();
	
	EventLoop();
	return result.Status();
}

int	Client::DirtyList(const ByteString &prefix, uint64_t count)
{
	return List(prefix, count, true);
}

int Client::ListP(const ByteString &prefix, uint64_t count, bool dirty)
{
	Command*	cmd;
	ByteString	args[2];
	DynArray<32> countString;
	
	countString.Writef("%U", count);
	
	args[0] = prefix;
	args[1] = countString;
	
	if (dirty)
	{
		cmd = CreateCommand(KEYSPACECLIENT_DIRTY_LISTP, false, 2, args);
		dirtyCommands.Add(cmd);
	}
	else
	{
		cmd = CreateCommand(KEYSPACECLIENT_LISTP, false, 2, args);
		safeCommands.Add(cmd);
	}
	
	result.Close();
	
	EventLoop();
	return result.Status();	
}

int Client::DirtyListP(const ByteString &prefix, uint64_t count)
{
	return ListP(prefix, count, true);
}

Result* Client::GetResult(int &status)
{
	status = result.Status();
	
	if (result.responses.Head())
		return &result;
	
	return NULL;
}

int Client::Set(const ByteString& key, const ByteString& value, bool submit)
{
	Command*	cmd;
	ByteString	args[2];
	int			status;
	
	args[0] = key;
	args[1] = value;
	
	cmd = CreateCommand(KEYSPACECLIENT_SET, submit, 2, args);
	safeCommands.Add(cmd);
	
	if (!submit)
		return KEYSPACE_OK;
	
	result.Close();
	EventLoop();
	status = result.Status();
	
	return status;
}

int Client::TestAndSet(const ByteString &key, const ByteString &test, const ByteString &value, bool submit)
{
	Command*	cmd;
	ByteString	args[3];
	int			status;
	
	args[0] = key;
	args[1] = test;
	args[2] = value;
	
	cmd = CreateCommand(KEYSPACECLIENT_TEST_AND_SET, submit, 3, args);
	safeCommands.Add(cmd);
	
	if (!submit)
		return KEYSPACE_OK;

	result.Close();
	EventLoop();
	status = result.Status();
	
	return status;
}

int Client::Add(const ByteString &key, int64_t num, int64_t &res, bool submit)
{
	Command*	cmd;
	ByteString	args[2];
	DynArray<32> numString;
	unsigned	nread;
	int			status;
	
	numString.Writef("%U", num);
	
	args[0] = key;
	args[1] = numString;
	
	cmd = CreateCommand(KEYSPACECLIENT_ADD, submit, 2, args);
	safeCommands.Add(cmd);
	
	if (!submit)
		return KEYSPACE_OK;

	result.Close();
	EventLoop();
	status = result.Status();
	
	if (status != KEYSPACE_OK)
	{
		result.Close();
		return status;
	}
	
	// TODO check conversion
	res = strntoint64(result.Value().buffer, result.Value().length, &nread);
	result.Close();
	return status;
}

int Client::Delete(const ByteString &key, bool submit)
{
	Command*	cmd;
	ByteString	args[1];
	int			status;
	
	args[0] = key;
	
	cmd = CreateCommand(KEYSPACECLIENT_DELETE, submit, 1, args);
	safeCommands.Add(cmd);
	
	if (!submit)
		return KEYSPACE_OK;

	result.Close();
	EventLoop();
	status = result.Status();
	
	return status;
}

int Client::Begin()
{
	if (!IsDone())
		return KEYSPACE_ERROR;

	result.Close();

	return KEYSPACE_OK;
}

int Client::Submit()
{
	Command**	it;
	
	it = safeCommands.Tail();
	if (it)
		(*it)->submit = true;
	else
		return KEYSPACE_OK;
	
	EventLoop();
	return result.Status();
}

void Client::EventLoop()
{
	while(!IsDone())
	{
		StateFunc();
		EventLoop::RunOnce();
	}
}

bool Client::IsDone()
{
	if (safeCommands.Length() == 0 &&
		dirtyCommands.Length() == 0 &&
		sentCommands.Length() == 0)
	{
		return true;
	}
	
	return false;
}

void Client::StateFunc()
{
	for (int i = 0; i < numConns; i++)
	{
		if (conns[i]->GetState() == ClientConn::DISCONNECTED &&
			conns[i]->disconnectTime + reconnectTimeout <= Now())
		{
			conns[i]->Connect(conns[i]->GetEndpoint(), timeout);
		}
	}
	
	if (safeCommands.Length() > 0 && master == -1)
	{
		if (masterTime && masterTime + timeout < Now())
		{
			DeleteCommands(safeCommands);
			result.Close();
			return;			
		}
		
		for (int i = 0; i < numConns; i++)
		{
			if (conns[i]->GetState() == ClientConn::CONNECTED &&
				!conns[i]->getMasterPending)
				{
					if (!conns[i]->getMasterTime || 
						conns[i]->getMasterTime + timeout < Now())
					{
						conns[i]->GetMaster();
					}
				}
		}
	}
	
	if (safeCommands.Length() > 0 && 
		master != -1 && 
		conns[master]->GetState() == ClientConn::CONNECTED)
	{
		SendCommand(conns[master], safeCommands);
	}
	
	
	if (dirtyCommands.Length() > 0)
	{
		int numTries = 3;
		while (distributeDirty && numTries > 0)
		{
			int i;
			i = (random() / (float) RAND_MAX) * numConns;
			if (conns[i]->GetState() == ClientConn::CONNECTED)
			{
				SendCommand(conns[i], dirtyCommands);
				return;
			}
			numTries--;
		}
		
		for (int i = 0; i < numConns; i++)
		{
			if (conns[i]->GetState() == ClientConn::CONNECTED)
			{
				SendCommand(conns[i], dirtyCommands);
				return;
			}
		}
	}
}

void Client::SendCommand(ClientConn* conn, CommandList& commands)
{
	Command**	it;
	
	it = commands.Head();
	sentCommands.Append(*it);
	
	conn->Send(**it);
	commands.Remove(it);
}

void Client::DeleteCommands(CommandList& commands)
{
	Command**	it;
	
	while ((it = commands.Head()) != NULL)
	{
		delete *it;
		commands.Remove(it);
	}
}

uint64_t Client::GetNextID()
{
	return cmdID++;
}

Command* Client::CreateCommand(char type, bool submit, int msgc, ByteString *msgv)
{
	Command*		cmd;
	int				len;
	char			tmp[20];
	char			idbuf[20];
	int				idlen;

	cmd = new Command;
	cmd->cmdID = GetNextID();
	cmd->type = type;
	cmd->submit = submit;
	
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
		cmd->msg.Append(msgv[i].buffer, msgv[i].length);
	}
	
	return cmd;
}

void Client::SetMaster(int master_)
{
	master = master_;
	masterTime = Now();
}
