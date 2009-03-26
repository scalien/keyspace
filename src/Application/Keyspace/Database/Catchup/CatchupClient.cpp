#include "CatchupClient.h"

void CatchupClient::Init(Table* table_)
{
	table = table_;
	
	table->Drop();
	//TODO: reset paxosID to 0, which is the default, empty db
}

void CatchupClient::OnRead()
{
	int msglength, nread, msgbegin, msgend;
	Endpoint endpoint;
	
	tcpread.requested = IO_READ_ANY;
	
	do
	{
		msglength = strntol(tcpread.data.buffer, tcpread.data.length, &nread);
		
		if (nread == 0 || tcpread.data.length <= nread)
			break;
			
		if (tcpread.data.buffer[nread] != ':')
			ASSERT_FAIL();
	
		msgbegin = nread + 1;
		msgend = nread + 1 + msglength;

		if (tcpread.data.length < msgend)
		{
			// request just the right amount
			tcpread.requested = msgend - tcpread.data.length;
			break;
		}

		if (msg.Read(ByteString(msglength, msglength, tcpread.data.buffer + msgbegin)))
			ProcessMsg();
		
		// move the rest back to the beginning of the buffer
		memcpy(tcpread.data.buffer, tcpread.data.buffer + msgend,
			tcpread.data.length - msgend);
		tcpread.data.length = tcpread.data.length - msgend;
		
		if (tcpread.data.length == 0)
			break;
	}
	while (true);
	
	ioproc->Add(&tcpread);

}

void CatchupClient::OnClose()
{
	Close();
}

void CatchupClient::ProcessMsg()
{
	if (msg.type == KEY_VALUE)
		OnKeyValue();
	else if (msg.type == DB_COMMAND)
		OnDBCommand();
	else if (msg.type == CATCHUP_COMMIT)
		OnCommit();
	else if (msg.type == CATCHUP_ROLLBACK)
		OnRollback();
	else
		ASSERT_FAIL();
}

void CatchupClient::OnKeyValue()
{
	table->Set(NULL, msg.key, msg.value);
}

void CatchupClient::OnDBCommand()
{
	paxosID = msg.paxosID;
	Execute(msg.paxosID, msg.dbCommand);
}

void CatchupClient::OnCommit()
{
}

void CatchupClient::OnRollback()
{
	table->Drop();
}

void CatchupClient::Execute(ulong64 paxosID, ByteString& dbCommand)
{
	//TODO:
}
