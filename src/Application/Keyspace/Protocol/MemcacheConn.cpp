#include "MemcacheConn.h"
#include "MemcacheServer.h"

#define CR					13
#define LF					10
#define CS_CR				"\015"
#define CS_LF				"\012"
#define CS_CRLF				CS_CR CS_LF
//#define CRLF_LENGTH			2
#define CRLF_LENGTH			strlen(newline)

#define MAX_TOKENS			10
#define MAX_MESSAGE_SIZE	64000

#define TOKEN_COMMAND		0
#define TOKEN_KEY			1
#define TOKEN_FLAGS			2
#define TOKEN_EXPTIME		3
#define	TOKEN_BYTES			4
#define TOKEN_NOREPLY		5
#define TOKEN_CAS_UNIQUE	5
#define TOKEN_CAS_NOREPLY	6


MemcacheConn::MemcacheConn() :
onRead(this, &MemcacheConn::OnRead),
onWrite(this, &MemcacheConn::OnWrite),
onClose(this, &MemcacheConn::OnClose)
{
	WriteBuffer* buffer;
	
	server = NULL;
	ioproc = NULL;
	kdb = NULL;
	newline = NULL;

	// preallocate a buffer for writeQueue
	buffer = new WriteBuffer;
	writeQueue.Append(buffer);
}

MemcacheConn::~MemcacheConn()
{
	WriteBuffer** it;
	
	for (it = writeQueue.Head(); it != NULL; it = writeQueue.Next(it))
		delete *it;
}

void MemcacheConn::Init(IOProcessor* ioproc_, KeyspaceDB* kdb_, MemcacheServer* server_)
{
	Log_Trace();

	server = server_;
	ioproc = ioproc_;
	kdb = kdb_;
	
	tcpread.fd = socket.fd;
	tcpread.data = readBuffer;
	tcpread.onComplete = &onRead;
	tcpread.onClose = &onClose;
	tcpread.requested = IO_READ_ANY;
	ioproc->Add(&tcpread);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onWrite;
	tcpwrite.onClose = &onClose;
	
	numpending = 0;
	closed = false;
	
}

void MemcacheConn::Write(const char *data, int count)
{
	Log_Trace();

	WriteBuffer* buf;
	WriteBuffer* head;
	
	head = *writeQueue.Head();
	buf = *writeQueue.Tail();
	if ((tcpwrite.active && head == buf) || buf->size - buf->length < count)
	{
		// TODO max queue size
		buf = new WriteBuffer;
		writeQueue.Append(buf);
	}
	
	memcpy(buf->buffer, data, count);
	buf->length = count;
	
	if (!tcpwrite.active)
		WritePending();
}

void MemcacheConn::TryClose()
{
	Log_Trace();

	if (!closed)
	{
		if (tcpread.active)
			ioproc->Remove(&tcpread);
		
		if (tcpwrite.active)
			ioproc->Remove(&tcpwrite);
		
		socket.Close();
	}

	closed = true;
	if (numpending == 0)
	{
		server->OnDisconnect(this);
		Log_Message("before delete this");
		delete this;		
	}
}

void MemcacheConn::OnRead()
{
	Log_Trace();

	const char *p;
	const char *last = NULL;
	int newlen;
	int remaining;

	p = last = tcpread.data.buffer;
	remaining = tcpread.data.length;

	while (true)
	{
		p = Process(p, remaining);
		if (!p)
		{
			TryClose();
			return;
		}
		
		if (p == last)
		{
			if (tcpread.data.length == tcpread.data.size)
			{
				Log_Message("message is bigger than buffer size, closing connection");
				TryClose();
				return;
			}
			newlen = tcpread.data.length - (p - tcpread.data.buffer);
			memmove(tcpread.data.buffer, p, newlen);
			tcpread.data.length = newlen;

			ioproc->Add(&tcpread);
			return;
		}
		
		remaining -= p - last;
		last = p;
	}
}

void MemcacheConn::OnWrite()
{
	Log_Trace();

	WriteBuffer** it;
	WriteBuffer* buf;
	WriteBuffer* last;
	
	it = writeQueue.Head();
	buf = *it;
	buf->Clear();
	tcpwrite.data.Clear();
	
	if (writeQueue.Size() > 1)
	{
		writeQueue.Remove(it);
		it = writeQueue.Tail();
		last = *it;
		if (last->length)
			writeQueue.Append(buf);
		else
			delete buf;
		
		WritePending();
	}
}

void MemcacheConn::OnClose()
{
	Log_Trace();

	TryClose();
}

void MemcacheConn::OnComplete(KeyspaceOp* op, bool status)
{
	Log_Trace();

	const char STORED[] = "STORED" CS_CRLF;
	const char NOT_STORED[] = "NOT_STORED" CS_CRLF;
	char buf[MAX_MESSAGE_SIZE];	// TODO can be bigger than that
	const char *p;
	int size;
	
	if (op->type == KeyspaceOp::GET && status)
	{
		size = snprintf(buf, sizeof(buf), "VALUE %.*s %d %d" CS_CRLF "%.*s" CS_CRLF "END" CS_CRLF, 
				 op->key.length, op->key.buffer,
				 0,
				 op->value.length,
				 op->value.length, op->value.buffer);
		
		p = buf;
	}
	else if (op->type == KeyspaceOp::SET)
	{
		if (status)
		{
			p = STORED;
			size = sizeof(STORED) - 1;			
		}
		else
		{
			p = NOT_STORED;
			size = sizeof(NOT_STORED) - 1;
		}
	}

	if (!closed)
		Write(p, size);

	numpending--;
}

int MemcacheConn::Tokenize(const char *data, int size, Token *tokens, int maxtokens)
{
	Log_Trace();

	const char *p;
	const char *token;
	int	numtoken = -1;
	int i = 0;
	
	p = data;
	token = data;

	// find CRLF and spaces between tokens
	while (p - data < size - 1)
	{
		if (p[0] == ' ' || (p[0] == CR))
		{
			// this is a hack so that it works from telnet
			if (p[1] != LF)
				newline = CS_CR;
			else
				newline = CS_CRLF;
			
			tokens[i].value = token;
			tokens[i].len = p - token;
			i++;
			token = p + 1;
			
			// this indicates that more space needed in tokens
			if (i == maxtokens)
				return maxtokens + 1;
		}
		
		if (p[0] == CR)
		{
			if (newline[1] != '\0' && p[1] == LF ||
				newline[1] == '\0')
			{
				numtoken = i;
				break;				
			}
		}
		
		p++;
	}
	
	return numtoken;
}

const char* MemcacheConn::Process(const char* data, int size)
{
	Log_Trace();

	Token tokens[MAX_TOKENS];
	int numtoken;
	
	numtoken = Tokenize(data, size, tokens, SIZE(tokens));
	// not enough data
	if (numtoken == -1)
		return data;
	
	// TODO allocate a bigger array (or choose better MAX_TOKENS value)
	if (numtoken == MAX_TOKENS + 1)
		return NULL;

	Log_Message("command = %.*s", tokens[TOKEN_COMMAND].len, tokens[TOKEN_COMMAND].value);
	
	if (numtoken >= 2 && 
		strncmp(tokens[TOKEN_COMMAND].value, "get", tokens[TOKEN_COMMAND].len) == 0 && 
		tokens[TOKEN_COMMAND].len == 3)
	{
		return ProcessGetCommand(data, size, tokens, numtoken);
	}
	else if ((numtoken == 5 || numtoken == 6) &&
			 strncmp(tokens[TOKEN_COMMAND].value, "set", tokens[TOKEN_COMMAND].len) == 0 &&
			 tokens[TOKEN_COMMAND].len == 3)
	{
		return ProcessSetCommand(data, size, tokens, numtoken);
	}
	
	// protocol error
	return NULL;
}

#define DATA_START(tokens, numtoken) (tokens[numtoken - 1].value + tokens[numtoken - 1].len + CRLF_LENGTH)

const char* MemcacheConn::ProcessGetCommand(const char* data, int size, Token* tokens, int numtoken)
{
	Log_Trace();

	const char *data_start;
	KeyspaceOp op;
	int i;
	
	data_start = DATA_START(tokens, numtoken);
	if (data_start > data + size)
		return data;

	op.client = this;
	op.type = KeyspaceOp::GET;
	for (i = 1; i < numtoken; i++)
	{
		op.key.buffer = (char *) tokens[i].value;
		op.key.size = tokens[i].len;
		op.key.length = tokens[i].len;
		
		op.value.Init();
		op.test.Init();
		
		Add(op);
	}
		
	return data_start;
}


const char* MemcacheConn::ProcessSetCommand(const char* data, int size, Token* tokens, int numtoken)
{
	Log_Trace();

	const char* data_start;
	KeyspaceOp op;
	long num;
	int numlen;
	
	data_start = DATA_START(tokens, numtoken);
	if (data_start > data + size)
		return data;
	
	op.client = this;
	op.type = KeyspaceOp::SET;
	
	numlen = 0;
	num = strntol((char *) tokens[TOKEN_BYTES].value, tokens[TOKEN_BYTES].len, &numlen);
	if (numlen != tokens[TOKEN_BYTES].len)
		return NULL;
	
	if (size - (data_start - data) < num + CRLF_LENGTH)
		return data;
	
	op.key.buffer = (char *) tokens[TOKEN_KEY].value;
	op.key.size = tokens[TOKEN_KEY].len;
	op.key.length = tokens[TOKEN_KEY].len;
	
	op.value.buffer = (char *) data_start;
	op.value.size = num;
	op.value.length = num;
	
	Add(op);
	
	return data_start + num + CRLF_LENGTH;
}

void MemcacheConn::Add(KeyspaceOp& op)
{
	Log_Trace();

	kdb->Add(op);
	numpending++;
}

void MemcacheConn::WritePending()
{
	Log_Trace();

	WriteBuffer* buf;
	
	buf = *writeQueue.Head();

	if (buf->length)
	{
		tcpwrite.data = *buf;
		tcpwrite.transferred = 0;

		ioproc->Add(&tcpwrite);		
	}
}
