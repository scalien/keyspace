#ifndef CONSOLE_H
#define CONSOLE_H

#include "Framework/Transport/TCPServer.h"

#define CONSOLE_SIZE	256

class Console;

class ConsoleConn : public TCPConn<CONSOLE_SIZE>
{
public:
	void			Init(Console* console);
	void			Disconnect();
	
private:
	Console*		console;
	char			endpointString[ENDPOINT_STRING_SIZE];
	
	// TCPConn interface
	virtual void	OnClose();
	virtual void	OnRead();
	virtual void	OnWrite();

	void			WritePrompt();
};

class ConsoleCommand
{
public:
	ConsoleCommand	*nextConsoleCommand;

	virtual			~ConsoleCommand() {}

	virtual void	Execute(const char *cmd,
							const char* args, ConsoleConn *conn) = 0;
};

class Console : public TCPServerT<Console, ConsoleConn, CONSOLE_SIZE>
{
public:
	void			Init(int port);
	void			Execute(const char* cmd,
							const char* args, ConsoleConn *conn);
	void			RegisterCommand(ConsoleCommand* command);

private:
	typedef Queue<ConsoleCommand, &ConsoleCommand::nextConsoleCommand> 
				CommandQueue;
	
	CommandQueue	commands;
};

#endif
