#ifndef IOOPERATION_H
#define IOOPERATION_H

#ifdef PLATFORM_LINUX
#include <aio.h>
#endif
#ifdef PLATFORM_DARWIN
#include <sys/aio.h>
#endif

#include <unistd.h>
#include "System/Events/Callable.h"
#include "System/Buffer.h"
#include "Endpoint.h"

/* all the cursor-type members of IOOperations are absolute cursors */

#define	TCP_READ	'a'
#define	TCP_WRITE	'b'
#define	UDP_READ	'x'
#define	UDP_WRITE	'y'
#define FILE_READ	'r'
#define FILE_WRITE	'w'

#define IO_READ_ANY -1

class AsyncOperation
{
public:
	AsyncOperation() { onComplete = NULL; }
	Callable*		onComplete;
};

class IOOperation
{
public:
	IOOperation()		{ fd = -1; onComplete = NULL; onClose = NULL; active = false; }

	ByteString		data;
	
	char			type;
	int				fd;
	int				offset;
	
	bool			active;

	Callable*		onComplete;
	Callable*		onClose;
};

class FileOp : public IOOperation
{
public:
	FileOp() : IOOperation() { offset = 0; nbytes = 0; }
	
	int				nbytes;
	
	struct aiocb	cb;
};

class FileWrite : public FileOp
{
public:
	FileWrite() : FileOp() { type = FILE_WRITE; }
};	


class FileRead : public FileOp
{
public:
	FileRead() : FileOp() { type = FILE_READ; }
};

class UDPWrite : public IOOperation
{
public:
	UDPWrite() : IOOperation() { type = UDP_WRITE; }
	
	Endpoint		endpoint;
};

class UDPRead : public IOOperation
{
public:
	UDPRead() : IOOperation() { type = UDP_READ; }
	
	Endpoint		endpoint;
};

class TCPWrite : public IOOperation
{
public:
	TCPWrite() : IOOperation() { type = TCP_WRITE; transferred = 0; }
	
	int				transferred;		/*	the IO subsystem has given the first 'transferred'
										    bytes to the kernel */
};

class TCPRead : public IOOperation
{
public:
	TCPRead() : IOOperation() { type = TCP_READ; listening = false; requested = 0; }
	
	bool			listening;		/*	it's a listener */
	int				requested;		/*	application is requesting the buffer to be filled
										exactly with a total of 'requested' bytes; if the
										application has no idea how much to expect, it sets
										'requested' to IO_READ_ANY; 'callback' is called when
										request bytes have been read into the buffer;
										useful for cases when header includes length */
};

#endif

