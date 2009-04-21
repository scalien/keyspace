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


void MemcacheConn::Init(MemcacheServer* server_, KeyspaceDB* kdb_)
{
	Log_Trace();
	
	TCPConn<>::Init();
	KeyspaceService::Init(kdb_);
	
	server = server_;

	newline = NULL;
	numpending = 0;
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
			Close();
			return;
		}
		
		if (p == last)
		{
			if (tcpread.data.length == tcpread.data.size)
			{
				Log_Message("message is bigger than buffer size, closing connection");
				Close();
				return;
			}
			newlen = tcpread.data.length - (p - tcpread.data.buffer);
			memmove(tcpread.data.buffer, p, newlen);
			tcpread.data.length = newlen;

			IOProcessor::Add(&tcpread);
			return;
		}
		
		remaining -= p - last;
		last = p;
	}
}

void MemcacheConn::OnClose()
{
	Log_Trace();

	Close();
	if (numpending == 0)
		server->DeleteConn(this);
}

void MemcacheConn::OnComplete(KeyspaceOp* op, bool status)
{
	Log_Trace();

	const char STORED[] = "STORED" CS_CRLF;
	const char NOT_STORED[] = "NOT_STORED" CS_CRLF;
	DynArray<MAX_MESSAGE_SIZE> buf;
	const char *p = buf.buffer;
	unsigned size = 0;
	
	if (op->type == KeyspaceOp::GET && status)
	{
		do {
			size = snprintf(buf.buffer, buf.size, "VALUE %.*s %d %d" CS_CRLF "%.*s" CS_CRLF "END" CS_CRLF, 
				 op->key.length, op->key.buffer,
				 0,
				 op->value.length,
				 op->value.length, op->value.buffer);
			if (size <= buf.size)
				break;
			buf.Reallocate(size, true);
		} while (1);
		p = buf.buffer;
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

	if (state == CONNECTED)
		Write(p, size);

	delete op;
	
	if (state == DISCONNECTED && numpending == 0)
		server->DeleteConn(this);
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
	KeyspaceOp *op;
	int i;
	
	data_start = DATA_START(tokens, numtoken);
	if (data_start > data + size)
		return data;

	op = new KeyspaceOp;
	op->service = this;
	op->type = KeyspaceOp::GET;
	for (i = 1; i < numtoken; i++)
	{
		op->key.buffer = (char *) tokens[i].value;
		op->key.size = tokens[i].len;
		op->key.length = tokens[i].len;
				
		if (!Add(op))
		{
			delete op;
			ASSERT_FAIL(); // todo
		}
	}
		
	return data_start;
}


const char* MemcacheConn::ProcessSetCommand(const char* data, int size, Token* tokens, int numtoken)
{
	Log_Trace();

	const char* data_start;
	KeyspaceOp *op;
	long num;
	unsigned numlen;
	
	data_start = DATA_START(tokens, numtoken);
	if (data_start > data + size)
		return data;

	op = new KeyspaceOp;	
	op->service = this;
	op->type = KeyspaceOp::SET;
	
	numlen = 0;
	num = strntoint64_t((char *) tokens[TOKEN_BYTES].value, tokens[TOKEN_BYTES].len, &numlen);
	if (numlen != (unsigned) tokens[TOKEN_BYTES].len)
		return NULL;
	
	if (size - (data_start - data) < num + (long) CRLF_LENGTH)
		return data;
	
	op->key.buffer = (char *) tokens[TOKEN_KEY].value;
	op->key.size = tokens[TOKEN_KEY].len;
	op->key.length = tokens[TOKEN_KEY].len;
	
	op->value.buffer = (char *) data_start;
	op->value.size = num;
	op->value.length = num;
	
	if (!Add(op))
	{
		delete op;
		ASSERT_FAIL(); // todo
	}
	
	return data_start + num + CRLF_LENGTH;
}
