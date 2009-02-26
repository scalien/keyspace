#include "MemcacheConn.h"
#include "MemcacheServer.h"

#define CR		13
#define LF		10
#define CS_CR	"\015"
#define CS_LF	"\012"
#define CS_CRLF	CS_CR CS_LF

#define MAX_TOKENS	10

MemcacheConn::MemcacheConn() :
onRead(this, &MemcacheConn::OnRead),
onWrite(this, &MemcacheConn::OnWrite),
onClose(this, &MemcacheConn::OnClose)
{
	WriteBuffer* buffer;
	
	server = NULL;
	ioproc = NULL;
	kdb = NULL;

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
		delete this;		
	}
}

void MemcacheConn::OnRead()
{
	Log_Trace();

	const char *p;
	int newlen;
	
	p = Process(tcpread.data.buffer, tcpread.data.length);
	if (!p)
	{
		TryClose();
		return;
	}

	if (p == tcpread.data.buffer)
	{
		// need more data
		ioproc->Add(&tcpread);
		return;
	}
	
	newlen = tcpread.data.length - (p - tcpread.data.buffer);
	memmove(tcpread.data.buffer, p, newlen);
	tcpread.data.length = newlen;
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
	
	it = writeQueue.Remove(it);
	if (it)
	{
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

	const char stored[] = "STORED" CS_CRLF;

	if (!closed)
		Write(stored, sizeof(stored) - 1);

	numpending--;
}

int MemcacheConn::Tokenize(const char *data, int size, Token *tokens, int maxtokens)
{
	Log_Trace();

	const char *p;
	const char *token;
	int	numtoken = -1;
	int i = 0;
	
	// find CRLF
	p = data;
	token = data;
	while (p - data < size - 1)
	{
		if (p[0] == ' ' || (p[0] == CR && p[1] == LF))
		{
			tokens[i].value = token;
			tokens[i].len = p - token;
			i++;
			token = p + 1;
			
			if (i > maxtokens)
				return -1;
		}
		
		if (p[0] == CR && p[1] == LF)
		{
			numtoken = i;
			break;			
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
	if (numtoken == -1)
		return data;

	if (numtoken >= 2 && strncmp(tokens[0].value, "get", tokens[0].len) == 0 && tokens[0].len == 3)
	{
		return ProcessGetCommand(data, size, tokens, numtoken);
	}
	else if ((numtoken == 5 || numtoken == 6) &&
			 strncmp(tokens[0].value, "set", tokens[0].len) == 0 &&
			 tokens[0].len == 3)
	{
		return ProcessSetCommand(data, size, tokens, numtoken);
	}
	
	return data;
}

#define DATA_START(tokens, numtoken) (tokens[numtoken - 1].value + tokens[numtoken - 1].len + 2)

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

	const char *data_start;
	KeyspaceOp op;
	long num;
	int numlen;
	
	data_start = DATA_START(tokens, numtoken);
	if (data_start > data + size)
		return data;
	
	op.client = this;
	op.type = KeyspaceOp::SET;
	
	numlen = 0;
	num = strntol((char *) tokens[4].value, tokens[4].len, &numlen);
	if (numlen != tokens[4].len)
		return NULL;
	
	if (size - (data_start - data) < num + 2)
		return data;
	
	op.key.buffer = (char *) tokens[1].value;
	op.key.size = tokens[1].len;
	op.key.length = tokens[1].len;
	
	op.value.buffer = (char *) data_start;
	op.value.size = num;
	op.value.length = num;
	
	Add(op);
	
	return data_start + num;
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
		ioproc->Add(&tcpwrite);		
	}
}