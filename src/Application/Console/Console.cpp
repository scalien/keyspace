#include "Console.h"
#include "System/Common.h"

#define CONN_BACKLOG	10

void ConsoleConn::Init(Console *console_)
{
	console = console_;
	TCPConn<CONSOLE_SIZE>::Init();
	socket.GetEndpoint(endpoint);
	
	WritePrompt();
	
	Log_Message("[%s] Console: client connected", endpoint.ToString());
}

void ConsoleConn::OnClose()
{
	Log_Message("[%s] Console: client disconnected", endpoint.ToString());

	Close();
	console->DeleteConn(this);
}

void ConsoleConn::OnRead()
{
	char*		end;
	char*		args;
	unsigned	pos;
	
	pos = 0;
	readBuffer.Append(tcpread.data.buffer, tcpread.data.length);
	
	while (
		(end = strnchr(readBuffer.buffer, '\r', readBuffer.length)) != NULL ||
		(end = strnchr(readBuffer.buffer, '\n', readBuffer.length)) != NULL)
	{
		// terminate the string in case of newline
		*end++ = '\0';
		if (end - readBuffer.buffer > 1)
		{
			Log_Message("[%s] Console: %s", endpoint.ToString(), readBuffer.buffer);
			
			args = strchr(readBuffer.buffer, ' ');
			while (args && *args == ' ')
				args++;
			console->Execute(readBuffer.buffer, args, this);
			
			WritePrompt();
		}
		
		readBuffer.Remove(0, end - readBuffer.buffer);
		pos += end - readBuffer.buffer;
		
	}
	
	tcpread.data.length -= pos;
	
	if (state != DISCONNECTED)
		AsyncRead();
}

void ConsoleConn::OnWrite()
{
	TCPConn<CONSOLE_SIZE>::OnWrite();
}

void ConsoleConn::WritePrompt()
{
	Write("> ", 2);
}

void Console::Init(int port_, const char* interface_)
{
	if (!TCPServerT<Console, ConsoleConn, CONSOLE_SIZE>::Init(port_, CONN_BACKLOG, interface_))
		Log_Message("Console: initialization failed, interface = %s, port = %d", interface_, port_);
}

void Console::Execute(const char* cmd, const char* args, ConsoleConn* conn)
{
	ConsoleCommand *cc;
	
	for (cc = commands.Head(); cc; cc = cc->nextConsoleCommand)
		cc->Execute(cmd, args, conn);
}

void Console::RegisterCommand(ConsoleCommand* command)
{
	commands.Append(command);
}

