#ifndef CONSOLE_H
#define CONSOLE_H

#include "Framework/Transport/TCPServer.h"

#define CONSOLE_SIZE	256

class Console;

class ConsoleConn : public TCPConn<CONSOLE_SIZE>
{
public:
	void				Init(Console* console);
	
private:
	Console*			console;
	Endpoint			endpoint;
	
	// TCPConn interface
	virtual void		OnClose();
	virtual void		OnRead();
	virtual void		OnWrite();

};

class ConsoleCommand
{
public:
	ConsoleCommand	*nextConsoleCommand;

	virtual			~ConsoleCommand() {}

	virtual void	Execute(const char *cmd, const char* args, ConsoleConn *conn) = 0;
};

class Console : public TCPServerT<Console, ConsoleConn, CONSOLE_SIZE>
{
public:
	void			Init(int port, const char* interface = "127.0.0.1");
	void			Execute(const char* cmd, const char* args, ConsoleConn *conn);
	void			RegisterCommand(ConsoleCommand* command);

private:
	typedef Queue<ConsoleCommand, &ConsoleCommand::nextConsoleCommand> CommandQueue;
	
	CommandQueue	commands;
};

#endif
