#ifndef IOOPERATION_H
#define IOOPERATION_H

#ifdef PLATFORM_DARWIN
#include <sys/types.h>
#endif

#include "System/Events/Callable.h"
#include "System/Buffer.h"
#include "Endpoint.h"

/* all the cursor-type members of IOOperations are absolute cursors */

#define	TCP_READ	'a'
#define	TCP_WRITE	'b'
#define	UDP_READ	'x'
#define	UDP_WRITE	'y'

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
	IOOperation()
	{
		fd = -1;
		active = false;
		pending = false;
		offset = 0;
		next = NULL;
		onComplete = NULL; 
		onClose = NULL; 
	}

	ByteString		data;
	
	char			type;
	int				fd;
	int				offset;
	
	bool			active;
	bool			pending;
	IOOperation*	next;

	Callable*		onComplete;
	Callable*		onClose;
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
	
	unsigned	transferred;		/*	the IO subsystem has given the first 'transferred'
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

