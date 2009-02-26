#include "MemoConn.h"

MemoConn::MemoConn(MemoDB* memodb_) :
onWelcome(this, &MemoConn::OnWelcome),
onRead(this, &MemoConn::OnRead),
onWrite(this, &MemoConn::OnWrite),
onCloseRead(this, &MemoConn::OnCloseRead),
onCloseWrite(this, &MemoConn::OnCloseWrite),
onMemoDBComplete(this, &MemoConn::OnMemoDBComplete)
{
	memodb = memodb_;
}

bool MemoConn::Init(IOProcessor* ioproc_)
{
	Log_Message("Bound socket %d", socket.fd);

	ioproc = ioproc_;
	
	tcpwrite.fd = socket.fd;
	tcpwrite.data = data;
	
	#define WELCOME_MSG "Welcome to MemoDB!\n\n"
	sprintf(tcpwrite.data.buffer, WELCOME_MSG);
	tcpwrite.data.length = strlen(WELCOME_MSG);
	tcpwrite.onComplete = &onWelcome;
	tcpwrite.onClose = &onCloseWrite;
	ioproc->Add(&tcpwrite);

	return true;
}

void MemoConn::OnWelcome()
{
	Log_Trace();
	
	tcpread.fd = socket.fd;
	tcpread.data = data;
	tcpread.requested = IO_READ_ANY;
	tcpread.onComplete = &onRead;
	tcpread.onClose = &onCloseRead;
	ioproc->Add(&tcpread);
	
	tcpwrite.fd = socket.fd;
	tcpwrite.onComplete = &onWrite;
}

void MemoConn::Shutdown()
{
	Log_Trace();

	socket.Close();
}

void MemoConn::OnRead()
{
	Log_Trace();
	
	int		nread, nwrite;
	char*	p;
	
	// todo: just for the telnet client
	if (tcpread.data.buffer[tcpread.data.length - 1] == '\n')
		tcpread.data.length--;
	if (tcpread.data.buffer[tcpread.data.length - 1] == '\r')
		tcpread.data.length--;

	bs = tcpread.data;
	nread = MemoClientMsg::Read(bs, &msg);

	// todo: fragmented messages not handled	
	if (nread > 0 && nread == bs.length)
	{
		p = bs.buffer + nread;
		
		msg.nodeID = memodb->NodeID();
		msg.msgID = memodb->msgID++;
		
		nwrite = MemoMsg::Write(&msg, p, bs.size - bs.length);
		bs.length += nwrite;
		
		memodb->Add(bs, &onMemoDBComplete);
	}
	else
	{
		// error parsing
		msg.Error();
		MemoClientMsg::Write(&msg, tcpwrite.data); // todo: retval
		if (tcpwrite.data.length > 0)
		{
			tcpwrite.transferred = 0;
			ioproc->Add(&tcpwrite);
		}
		else
		{
			tcpread.data.length = 0;
			ioproc->Add(&tcpread);
		}
	}
}

void MemoConn::OnWrite()
{
	Log_Trace();
	
	tcpread.data.length = 0;
	ioproc->Add(&tcpread);
}

void MemoConn::OnCloseRead()
{
	Log_Trace();
	
	Shutdown();
	delete this;
}

void MemoConn::OnCloseWrite()
{
	Log_Trace();
	
	Shutdown();
	delete this;
}

void MemoConn::OnMemoDBComplete()
{
	MemoClientMsg::Write(&memodb->msg, tcpwrite.data);	
	
	if (tcpwrite.data.length < 1)
	{
		msg.Error();
		MemoClientMsg::Write(&msg, tcpwrite.data);
	}
	
	if (tcpwrite.data.length > 0)
	{
		tcpwrite.transferred = 0;
		ioproc->Add(&tcpwrite);
	}
	else
	{
		tcpread.data.length = 0;
		ioproc->Add(&tcpread);
	}
}
