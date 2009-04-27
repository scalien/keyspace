#include <inttypes.h>
#include <time.h>
#include "System/Time.h"
#include "KeyspaceClient.h"
#include "../Protocol/Keyspace/KeyspaceClientMsg.h"

#define RECONNECT_TIMEOUT	1000


static char* strnchr(const char* s, int c, int len)
{
	const char* p;
	
	p = s;
	while (p - s < len)
	{
		if (*p == c)
			return (char*) p;
		p++;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////
//
// Result implementation
//
/////////////////////////////////////////////////////////////////////

KeyspaceClient::Result::Result(KeyspaceClient &client_) :
client(client_)
{
	id = 0;
	type = 0;
	status = KEYSPACE_OK;
}

KeyspaceClient::Result::~Result()
{
}

void KeyspaceClient::Result::Close()
{
	id = 0;
	type = 0;
	key.Clear();
	value.Clear();
	status = KEYSPACE_OK;
}

KeyspaceClient::Result* KeyspaceClient::Result::Next(int &status_)
{
	ByteString	resp;
	int			ret;
	
	if (id == 0)
	{
		status_ = KEYSPACE_ERROR;
		return NULL;
	}
	
	ret = client.Read(resp);
	if (ret < 0)
	{
		Close();
		status_ = KEYSPACE_ERROR;
		return NULL;
	}
	
	if (type == KEYSPACECLIENT_GET)
		ret = ParseValueResponse(resp);
	else if (type == KEYSPACECLIENT_LIST)
		ret = ParseListResponse(resp);
	else if (type == KEYSPACECLIENT_LISTP)
		ret = ParseListPResponse(resp);
	
	client.readBuf.Remove(0, resp.length);

	if (ret < 0)
	{
		Close();
		status_ = ret;
		return NULL;
	}

	if (ret == 0)
	{
		Close();
		status_ = KEYSPACE_OK;
		return NULL;
	}
	
	status_ = status = KEYSPACE_OK;
	
	return this;
}

const ByteString& KeyspaceClient::Result::Key()
{
	return key;
}

const ByteString& KeyspaceClient::Result::Value()
{
	return value;
}

int KeyspaceClient::Result::Status()
{
	return status;
}

int KeyspaceClient::Result::ParseValueResponse(const ByteString &resp)
{
	const char*		colon;
	const char*		sid;
	const char*		vallen;
	const char*		code;
	uint64_t		respId;
	unsigned		nread;
	int				len;
	
	// length of the full response
	colon = strnchr(resp.buffer, ':', resp.length);
	if (!colon)
		return KEYSPACE_ERROR;
	
	colon++;
	// response code
	code = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return KEYSPACE_ERROR;
	
	if (code[0] == KEYSPACECLIENT_NOTMASTER)
		return KEYSPACE_NOTMASTER;
	if (code[0] == KEYSPACECLIENT_FAILED)
		return KEYSPACE_FAILED;
	
	if (code[0] != KEYSPACECLIENT_OK)
		return KEYSPACE_ERROR;
	
	colon++;
	// response id
	sid = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return KEYSPACE_ERROR;

	respId = strntoint64_t(sid, colon - sid, &nread);
	if (respId != id)
		return KEYSPACE_ERROR;
	
	colon++;
	// length of value
	vallen = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return false;
	
	len = (int) strntoint64_t(vallen, colon - vallen, &nread);
	if (nread < 1)
		return KEYSPACE_ERROR;
	
	colon++;
	if ((unsigned) len != resp.length - (colon - resp.buffer))
		return KEYSPACE_ERROR;
	
	value.Clear();
	value.Append(colon, len);
	
	return len;
}

int KeyspaceClient::Result::ParseListResponse(const ByteString &resp)
{
	const char*		colon;
	const char*		sid;
	const char*		vallen;
	const char*		code;
	uint64_t		respId;
	unsigned		nread;
	int				len;
	
	// length of the full response
	colon = strnchr(resp.buffer, ':', resp.length);
	if (!colon)
		return KEYSPACE_ERROR;
	
	colon++;
	// response code
	code = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return KEYSPACE_ERROR;

	if (code[0] != KEYSPACECLIENT_LISTITEM && code[0] != KEYSPACECLIENT_LISTEND)
		return KEYSPACE_ERROR;
	
	if (code[0] == KEYSPACECLIENT_LISTEND)
		return KEYSPACE_OK;
	
	colon++;
	// response id
	sid = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return KEYSPACE_ERROR;

	respId = strntoint64_t(sid, colon - sid, &nread);
	if (respId != id)
		return KEYSPACE_ERROR;
	
	colon++;
	// length of value
	vallen = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return false;
	
	len = (int) strntoint64_t(vallen, colon - vallen, &nread);
	if (nread < 1)
		return KEYSPACE_ERROR;
	
	colon++;
	if ((unsigned) len != resp.length - (colon - resp.buffer))
		return KEYSPACE_ERROR;
	
	key.Clear();
	key.Append(colon, len);
	
	return len;
}

int KeyspaceClient::Result::ParseListPResponse(const ByteString &resp)
{
	const char*		colon;
	const char*		sid;
	const char*		vallen;
	const char*		code;
	uint64_t		respId;
	unsigned		nread;
	int				len;
	
	// length of the full response
	colon = strnchr(resp.buffer, ':', resp.length);
	if (!colon)
		return KEYSPACE_ERROR;
	
	colon++;
	// response code
	code = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return KEYSPACE_ERROR;

	if (code[0] != KEYSPACECLIENT_LISTPITEM && code[0] != KEYSPACECLIENT_LISTEND)
		return KEYSPACE_ERROR;

	if (code[0] == KEYSPACECLIENT_LISTEND)
		return KEYSPACE_OK;
	
	colon++;
	// response id
	sid = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return KEYSPACE_ERROR;

	respId = strntoint64_t(sid, colon - sid, &nread);
	if (respId != id)
		return KEYSPACE_ERROR;
	
	colon++;
	// length of key
	vallen = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return false;
	
	len = (int) strntoint64_t(vallen, colon - vallen, &nread);
	if (nread < 1)
		return KEYSPACE_ERROR;

	colon += nread;
	key.Clear();
	key.Append(colon, len);
	
	colon += len + 1;
	// length of value
	vallen = colon;
	colon = strnchr(colon, ':', resp.length - (colon - resp.buffer));
	if (!colon)
		return false;
	
	len = (int) strntoint64_t(vallen, colon - vallen, &nread);
	if (nread < 1)
		return KEYSPACE_ERROR;

	colon += nread;
	if ((unsigned) len != resp.length - (colon - resp.buffer))
		return KEYSPACE_ERROR;

	value.Clear();
	value.Append(colon, len);
	
	
	return len;
}


/////////////////////////////////////////////////////////////////////
//
// KeyspaceClient implementation
//
/////////////////////////////////////////////////////////////////////

KeyspaceClient::KeyspaceClient() :
result(*this)
{
	endpoints = NULL;
}

int KeyspaceClient::Init(int nodec, char* nodev[], uint64_t timeout_)
{
	timeout = timeout_;
	numEndpoints = nodec;

	connectMaster = false;
	endpoint = NULL;

	delete[] endpoints;
	endpoints = new Endpoint[nodec];
	for (int i = 0; i < numEndpoints; i++)
		endpoints[i].Set(nodev[i]);
	
	id = Now();
	startId = 0;
	numPending = 0;
	srand(time(NULL));
	return Reconnect();
}

KeyspaceClient::~KeyspaceClient()
{
	delete[] endpoints;
}

int KeyspaceClient::ConnectMaster()
{
	int			master = KEYSPACE_ERROR;
	uint64_t	starttime;
	
	connectMaster = true;
	if (timeout)
		starttime = Now();

	while (master < 0 && (timeout == 0 || Now() - starttime < timeout))
	{
		master = GetMaster();
		if (master >= 0 && endpoint == &endpoints[master])
			break;

		Disconnect();
		if (master >= 0)
		{
			if (master >= numEndpoints)
				return KEYSPACE_ERROR;

			if (Connect(master))
				return master;
		}
		
		master = KEYSPACE_ERROR;
	}
	return master;
}

/////////////////////////////////////////////////////////////////////
//
// read commands
//
/////////////////////////////////////////////////////////////////////

// GetMaster is special because it combines application logic with low-level
// connect logic.
int KeyspaceClient::GetMaster()
{
	int				master = -1;
	unsigned		nread;
	int				ret;
	uint64_t		starttime;
	DynArray<32>	msg;
	DynArray<32>	cmd;
	ByteArray<CS_INT_SIZE(int)>	resp;
	
	cmd.Printf("%c:%" PRIu64 "", KEYSPACECLIENT_GETMASTER, GetNextID());
	msg.Printf("%u:%.*s", cmd.length, cmd.length, cmd.buffer);
	
	starttime = Now();
	while (master < 0 && (timeout == 0 || Now() - starttime < timeout))
	{
		while (!endpoint)
		{
			Disconnect();
			if (ConnectRandom())
				break;
			if (timeout && Now() - starttime > timeout)
				return KEYSPACE_ERROR;
		}

		Log_Trace("sending %.*s, node = %d", msg.length, msg.buffer, endpoint - endpoints);
		ret = socket.Send(msg.buffer, msg.length, timeout);
		if ((unsigned) ret < msg.length)
		{
			Disconnect();
			continue;
		}
			
		if (GetValueResponse(resp) < 0)
		{
			Disconnect();
			continue;
		}
			
		master = (int) strntoint64_t(resp.buffer, resp.length, &nread);
		if (nread < 1)
		{
			master = -1;
			Disconnect();
			continue;
		}
		
		return master;
	}
	
	return KEYSPACE_ERROR;
}

int KeyspaceClient::Get(const ByteString &key, ByteString &value, bool dirty)
{
	ByteString		args[1];
	char			cmd;
	
	args[0] = key;
	
	if (dirty)
		cmd = KEYSPACECLIENT_DIRTYGET;
	else
		cmd = KEYSPACECLIENT_GET;

	if (SendMessage(cmd, false, 1, args) < 0)
		return KEYSPACE_ERROR;

	return GetValueResponse(value);
}

int KeyspaceClient::DirtyGet(const ByteString &key, ByteString &value)
{
	return Get(key, value, true);
}

/////////////////////////////////////////////////////////////////////
//
// read commands with result
//
/////////////////////////////////////////////////////////////////////

int KeyspaceClient::Get(const ByteString &key, bool dirty)
{
	ByteString		args[1];
	char			cmd;

	// there is an active operation
	if (result.id != 0)
		return KEYSPACE_ERROR;
	
	args[0] = key;
	
	if (dirty)
		cmd = KEYSPACECLIENT_DIRTYGET;
	else
		cmd = KEYSPACECLIENT_GET;

	if (SendMessage(cmd, false, 1, args) < 0)
		return KEYSPACE_ERROR;

	result.id = id;
	result.type = KEYSPACECLIENT_GET;
	result.key.Append(key.buffer, key.length);
	
	return KEYSPACE_OK;
}

int KeyspaceClient::DirtyGet(const ByteString &key)
{
	return Get(key, true);
}

int	KeyspaceClient::DirtyList(const ByteString &prefix, uint64_t count)
{
	return List(prefix, count, true);
}

int KeyspaceClient::List(const ByteString &prefix, uint64_t count, bool dirty)
{
	ByteString		args[2];
	ByteString		resp;
	char			cmd;
	ByteArray<CS_INT_SIZE(count)> scount;

	// there is an active operation
	if (result.id != 0)
		return KEYSPACE_ERROR;
	
	scount.Printf("%" PRIu64, count);
	
	args[0] = prefix;
	args[1] = scount;
	
	if (dirty)
		cmd = KEYSPACECLIENT_DIRTYLIST;
	else
		cmd = KEYSPACECLIENT_LIST;
	
	if (SendMessage(cmd, false, 2, args) < 0)
		return KEYSPACE_ERROR;

	result.id = id;
	result.type = KEYSPACECLIENT_LIST;
	
	return KEYSPACE_OK;
}

int	KeyspaceClient::DirtyListP(const ByteString &prefix, uint64_t count)
{
	return ListP(prefix, count, true);
}

int KeyspaceClient::ListP(const ByteString &prefix, uint64_t count, bool dirty)
{
	ByteString		args[2];
	ByteString		resp;
	char			cmd;
	ByteArray<CS_INT_SIZE(count)> scount;

	// there is an active operation
	if (result.id != 0)
		return KEYSPACE_ERROR;
	
	scount.Printf("%" PRIu64, count);
	
	args[0] = prefix;
	args[1] = scount;
	
	if (dirty)
		cmd = KEYSPACECLIENT_DIRTYLISTP;
	else
		cmd = KEYSPACECLIENT_LISTP;
	
	if (SendMessage(cmd, false, 2, args) < 0)
		return KEYSPACE_ERROR;

	result.id = id;
	result.type = KEYSPACECLIENT_LISTP;

	return KEYSPACE_OK;
}

KeyspaceClient::Result* KeyspaceClient::GetResult(int &status)
{
	status = KEYSPACE_ERROR;
	if (result.id == 0)
		return NULL;
	
	status = KEYSPACE_OK;
	ResetReadBuffer();
	return result.Next(status);
}

/////////////////////////////////////////////////////////////////////
//
// write commands
//
/////////////////////////////////////////////////////////////////////

int KeyspaceClient::Set(const ByteString &key, const ByteString &value, bool submit)
{
	ByteString		args[2];

	if (!submit && startId == 0)
		return KEYSPACE_ERROR;
	
	args[0] = key;
	args[1] = value;
	
	if (SendMessage(KEYSPACECLIENT_SET, submit, 2, args) < 0)
		return KEYSPACE_ERROR;
	
	if (!submit)
	{
		numPending++;
		return KEYSPACE_OK;
	}
	
	return GetStatusResponse();
}

int KeyspaceClient::TestAndSet(const ByteString &key, const ByteString &test, const ByteString &value, bool submit)
{
	ByteString		args[3];

	if (!submit && startId == 0)
		return KEYSPACE_ERROR;
	
	args[0] = key;
	args[1] = test;
	args[2] = value;
	
	if (SendMessage(KEYSPACECLIENT_TESTANDSET, submit, 3, args) < 0)
		return KEYSPACE_ERROR;

	if (!submit)
	{
		numPending++;
		return KEYSPACE_OK;
	}

	return GetStatusResponse();
}

int KeyspaceClient::Add(const ByteString &key, int64_t num, int64_t &result, bool submit)
{
	ByteString		args[2];
	unsigned		nread;
	ByteArray<CS_INT_SIZE(num)>		snum;
	ByteArray<CS_INT_SIZE(result)>	resp;
	
	if (!submit && startId == 0)
		return KEYSPACE_ERROR;

	snum.Printf("%" PRIu64, num);
	
	args[0] = key;
	args[1] = snum;
	
	if (SendMessage(KEYSPACECLIENT_ADD, submit, 2, args) < 0)
		return KEYSPACE_ERROR;

	if (!submit)
	{
		numPending++;
		return KEYSPACE_OK;
	}

	if (GetValueResponse(resp) < 0)
		return KEYSPACE_ERROR;
	
	result = strntoint64_t(resp.buffer, resp.length, &nread);
	if (nread < 1)
		return KEYSPACE_ERROR;
		
	return KEYSPACE_OK;
}

int KeyspaceClient::Delete(const ByteString &key, bool submit)
{
	ByteString		args[1];

	if (!submit && startId == 0)
		return KEYSPACE_ERROR;
	
	args[0] = key;
	
	if (SendMessage(KEYSPACECLIENT_DELETE, submit, 1, args) < 0)
		return KEYSPACE_ERROR;

	if (!submit)
	{
		numPending++;
		return KEYSPACE_OK;
	}

	return GetStatusResponse();
}

int KeyspaceClient::Begin()
{
	if (numPending > 0)
		return KEYSPACE_ERROR;

	numPending = 0;
	startId = id + 1;
	
	return KEYSPACE_OK;
}

int KeyspaceClient::Submit()
{
	ByteString	msg;

	if (startId == 0 || numPending == 0)
		return KEYSPACE_OK;

	Log_Trace("sending 1:*, node = %d", endpoint - endpoints);
	if (socket.Send("1:*", 3, timeout) < 0)
	{
		startId = 0;
		numPending = 0;
		return KEYSPACE_ERROR;
	}

	ResetReadBuffer();
	
	while (numPending > 0)
	{
		if (Read(msg) < 0)
			return KEYSPACE_ERROR;
		
		Log_Trace("%.*s", msg.length, msg.buffer);
		readBuf.Remove(0, (msg.buffer - readBuf.buffer) + msg.length);

		numPending--;
	}
	
	startId = 0;
	
	return KEYSPACE_OK;
}

/////////////////////////////////////////////////////////////////////
//
// KeyspaceClient private implementation
//
/////////////////////////////////////////////////////////////////////


uint64_t KeyspaceClient::GetNextID()
{
	return ++id;
}

int KeyspaceClient::Reconnect()
{
	bool		ret;
	uint64_t	starttime;
	
	starttime = Now();
	while (timeout == 0 || Now() - starttime < timeout)
	{
		if (connectMaster && ConnectMaster() >= 0)
			ret = true;
		else
			ret = ConnectRandom();

		if (ret)
			return KEYSPACE_OK;
		Sleep(RECONNECT_TIMEOUT);
	}
	
	return KEYSPACE_ERROR;
}

bool KeyspaceClient::ConnectRandom()
{
	int rnd;
	
	rnd = rand();
	rnd &= (numEndpoints - 1);
	return Connect(rnd);
}

bool KeyspaceClient::Connect(int n)
{
	endpoint = &endpoints[n];
	socket.Close();
	socket.Create(TCP);
	socket.SetNonblocking();
	return socket.Connect(*endpoint);		
}

void KeyspaceClient::Disconnect()
{
	if (endpoint)
	{
		socket.Close();
		endpoint = NULL;
	}
}

int KeyspaceClient::SendMessage(char cmd, bool submit, int msgc, const ByteString *msgv)
{
	DynArray<128>	msg;
	int				len;
	char			tmp[20];
	char			idbuf[20];
	int				idlen;
	uint64_t		cmdID;
	
	cmdID = GetNextID();
	
	len = 2; // command + colon
	idlen = snprintf(idbuf, sizeof(idbuf), "%" PRIu64, cmdID);
	len += idlen;
	for (int i = 0; msgv && i < msgc; i++)
		len += 1 + NumLen(msgv[i].length) + 1 + msgv[i].length;
	
	len = snprintf(tmp, sizeof(tmp), "%d:", len);
	msg.Append(tmp, len);
	
	msg.Append(&cmd, 1);
	msg.Append(":", 1);
	msg.Append(idbuf, idlen);
	
	for (int i = 0; msgv && i < msgc; i++)
	{
		len = snprintf(tmp, sizeof(tmp), ":%d:", msgv[i].length);
		msg.Append(tmp, len);
		msg.Append(msgv[i].buffer, msgv[i].length);
	}
	
	if (submit)
		msg.Append("1:*", 3);
	
	return Send(msg);
}

int KeyspaceClient::Send(const ByteString &msg)
{
	int			ret;
	uint64_t	starttime;
	
	starttime = Now();
	
//	Log_Trace("sending %.*s, node = %d", msg.length, msg.buffer, endpoint - endpoints);
	while (timeout == 0 || Now() - starttime < timeout)
	{
		Log_Trace("sending %.*s, node = %d", msg.length, msg.buffer, endpoint - endpoints);
		ret = socket.Send(msg.buffer, msg.length, timeout);
		if (ret == (int) msg.length)
			return KEYSPACE_OK;
		
		Disconnect();
		Reconnect();
	}
	
	return KEYSPACE_ERROR;
}

void KeyspaceClient::ResetReadBuffer()
{
	readBuf.Clear();
}

int KeyspaceClient::ReadMessage(ByteString &msg)
{
	char*		colon;
	int			length;
	int			slength;
	unsigned	nread;
	
	colon = strnchr(readBuf.buffer, ':', readBuf.length);
	if (!colon)
		return -1;
	
	slength = (int) strntoint64_t(readBuf.buffer, colon - readBuf.buffer, &nread);
	if (nread < 0)
		return -1;
	
	length = colon - readBuf.buffer + 1 + slength;
	if (readBuf.length < (unsigned) length)
		return -1;
	
	msg.buffer = readBuf.buffer;
	msg.length = length;
	msg.size = length;
	
	return length;
}

int KeyspaceClient::Read(ByteString &msg)
{
	char		buf[4096];
	int			ret;
	
	while (true)
	{
		ret = ReadMessage(msg);
		if (ret >= 0)
			return ret;
		
		ret = socket.Read(buf, sizeof(buf), timeout);
		if (ret <= 0)
			return KEYSPACE_ERROR;
		
		readBuf.Append(buf, ret);
	}
}

int KeyspaceClient::GetValueResponse(ByteString &resp)
{
	ByteString		msg;
	const char*		colon;
	const char*		sid;
	const char*		vallen;
	const char*		code;
	uint64_t		respId;
	unsigned		nread;
	int				len;
	
	ResetReadBuffer();
	if (Read(msg) < 0)
		return KEYSPACE_ERROR;

	Log_Trace("%.*s", msg.length, msg.buffer);
	
	// length of the full response
	colon = strnchr(msg.buffer, ':', msg.length);
	if (!colon)
		return KEYSPACE_ERROR;
	
	colon++;
	// response code
	code = colon;
	colon = strnchr(colon, ':', msg.length - (colon - msg.buffer));
	if (!colon)
		return KEYSPACE_ERROR;
	
	if (code[0] == KEYSPACECLIENT_NOTMASTER)
		return KEYSPACE_NOTMASTER;
	if (code[0] == KEYSPACECLIENT_FAILED)
		return KEYSPACE_FAILED;
	
	if (code[0] != KEYSPACECLIENT_OK)
		return KEYSPACE_ERROR;
	
	colon++;
	// response id
	sid = colon;
	colon = strnchr(colon, ':', msg.length - (colon - msg.buffer));
	if (!colon)
		return KEYSPACE_ERROR;

	respId = strntoint64_t(sid, colon - sid, &nread);
	if (respId != id)
		return KEYSPACE_ERROR;
	
	colon++;
	// length of value
	vallen = colon;
	colon = strnchr(colon, ':', msg.length - (colon - msg.buffer));
	if (!colon)
		return false;
	
	len = (int) strntoint64_t(vallen, colon - vallen, &nread);
	if (nread < 1)
		return KEYSPACE_ERROR;
	
	colon++;
	if ((unsigned) len != msg.length - (colon - msg.buffer))
		return KEYSPACE_ERROR;
	
	if (resp.size < (unsigned) len)
		return len;

	memcpy(resp.buffer, colon, len);
	resp.length = len;
	
	return len;
}

int KeyspaceClient::GetStatusResponse()
{
	ByteString		msg;
	const char*		colon;
	const char*		code;
	uint64_t		cmdID;
	unsigned		nread;
	
	ResetReadBuffer();
	if (Read(msg) < 0)
		return KEYSPACE_ERROR;

	Log_Trace("%.*s", msg.length, msg.buffer);
	
	// length of the full response
	colon = strnchr(msg.buffer, ':', msg.length);
	if (!colon)
		return KEYSPACE_ERROR;
	
	colon++;
	// response code
	code = colon;
	colon = strnchr(colon, ':', msg.length - (colon - msg.buffer));
	if (!colon)
		return KEYSPACE_ERROR;

	if (code[0] == KEYSPACECLIENT_NOTMASTER)
		return KEYSPACE_NOTMASTER;
	
	if (code[0] == KEYSPACECLIENT_FAILED)
		return KEYSPACE_FAILED;
	
	if (code[0] != KEYSPACECLIENT_OK)
		return KEYSPACE_ERROR;
	
	colon = strnchr(msg.buffer, ':', msg.length);
	if (!colon)
		return KEYSPACE_ERROR;

	// TODO compare ids
	colon++;
	cmdID = (uint64_t) strntouint64_t(colon, msg.length - (colon - msg.buffer), &nread);
	if (nread < 1)
		return KEYSPACE_ERROR;
	
	if (id != cmdID)
		return KEYSPACE_ERROR;
	
	return KEYSPACE_OK;
}

