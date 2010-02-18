#ifdef PLATFORM_WINDOWS

#include "IOProcessor.h"
#include "System/Platform.h"
#include "System/Containers/List.h"
#include "System/Common.h"
#include "System/Log.h"
#include "System/Events/EventLoop.h"

#include <winsock2.h>
#include <mswsock.h>

// the address buffer passed to AcceptEx() must be 16 more than the max address length (see MSDN)
#define ACCEPT_ADDR_LEN		(sizeof(sockaddr_in) + 16)
#define MAX_TCP_READ		8192

// this structure is used for storing Windows/IOCP specific data
struct IODesc
{
	IOOperation*	read;
	IOOperation*	write;

	OVERLAPPED		ovlRead;			// for accept/recv
	OVERLAPPED		ovlWrite;			// for connect/send

	FD				acceptFd;			// with a listening socket, the accepted FD is stored here
	byte			acceptData[2 * ACCEPT_ADDR_LEN];	// TODO allocate dynamically

	IODesc*			next;				// pointer for free list handling
};

static HANDLE	iocp = NULL;			// global completion port handle
static IODesc*	iods;					// pointer to allocated array of IODesc's
static IODesc*	freeIods;				// pointer to the free list of IODesc's
static IODesc	callback;				// special IODesc for handling IOProcessor::Complete events
const FD		INVALID_FD = {-1, INVALID_SOCKET};	// special FD to indicate invalid value
unsigned		SEND_BUFFER_SIZE = 65537;
static volatile bool terminated = false;
static unsigned		numIOProcClients = 0;

static LPFN_CONNECTEX	ConnectEx;

bool ProcessTCPRead(TCPRead* tcpread);
bool ProcessUDPRead(UDPRead* udpread);
bool ProcessTCPWrite(TCPWrite* tcpwrite);
bool ProcessUDPWrite(UDPWrite* udpwrite);


// helper function for FD -> IODesc mapping
static IODesc* GetIODesc(const FD& fd)
{
	return &iods[fd.index];
}

// return the asynchronously accepted FD
bool IOProcessorAccept(const FD& listeningFd, FD& fd)
{
	IODesc*		iod;

	iod = GetIODesc(listeningFd);
	fd = iod->acceptFd;

	// this need to be called so that getpeername works
	setsockopt(fd.sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char *)&listeningFd.sock, sizeof(SOCKET));

	return true;
}

// asynchronous connect
bool IOProcessorConnect(FD& fd, Endpoint& endpoint)
{
	sockaddr_in	localAddr;
	IODesc*		iod;

	memset(&localAddr, 0, sizeof(sockaddr_in));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	// the socket must be bound so that ConnectEx works
	if (bind(fd.sock, (sockaddr*) &localAddr,  sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		// WSAINVAL means it is already bound
		if (WSAGetLastError() != WSAEINVAL)
			return false;
	}

	iod = GetIODesc(fd);
	memset(&iod->ovlWrite, 0, sizeof(OVERLAPPED));
	if (!ConnectEx(fd.sock, (sockaddr*) endpoint.GetSockAddr(), ENDPOINT_SOCKADDR_SIZE, NULL, 0, NULL, &iod->ovlWrite))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
			return false;
	}

	return true;
}

// register FD with an IODesc, also register in completion port
bool IOProcessorRegisterSocket(FD& fd)
{
	IODesc*		iod;

	assert(freeIods != NULL);

	iod = freeIods;
	// unlink iod from free list
	freeIods = iod->next;
	iod->next = NULL;
	iod->read = NULL;
	iod->write = NULL;

	fd.index = iod - iods;

	CreateIoCompletionPort((HANDLE)fd.sock, iocp, (ULONG_PTR)iod, 0);

	Log_Trace("fd = %d", fd.index);
		
	return true;
}

// put back the IODesc to the free list and cancel all IO and put back FD
bool IOProcessorUnregisterSocket(FD& fd)
{
	IODesc*		iod;
	BOOL		ret;

	Log_Trace("fd = %d", fd.index);

	iod = &iods[fd.index];
	iod->next = freeIods;
	freeIods = iod;

	ret = CancelIo((HANDLE)fd.sock);
	if (ret == 0)
	{
		ret = WSAGetLastError();
		Log_Trace("CancelIo error %d", ret);
		return false;
	}

	return true;
}

BOOL WINAPI ConsoleCtrlHandler(DWORD /*ctrlType*/)
{
	if (terminated)
	{
		STOP_FAIL("aborting due to user request", 0);
		return FALSE;
	}
		
	terminated = true;
	IOProcessor::Complete(NULL);
	return TRUE;
}

bool IOProcessor::Init(int maxfd)
{
	WSADATA		wsaData;
	SOCKET		s;
	GUID		guid = WSAID_CONNECTEX;
	DWORD		bytesReturned;

	if (iocp)
		return true;

	// initialize a Console Control Handler routine
#ifndef KEYSPACE_CLIENTLIB
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
#endif

	// initialize Winsock2 library
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		return false;

	// create a global completion port object
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

	// create a dummy socket to pass to WSAIoctl()
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!s)
		return false;

	// get ConnectEx function pointer
	if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid, sizeof(GUID), &ConnectEx, sizeof(LPFN_CONNECTEX),
		&bytesReturned, NULL, NULL) == SOCKET_ERROR)
	{
		return false;
	}
	closesocket(s);

	// create an array for IODesc indexing
	iods = new IODesc[maxfd];
	for (int i = 0; i < maxfd - 1; i++)
		iods[i].next = &iods[i + 1];

	iods[maxfd - 1].next = NULL;
	freeIods = iods;

	numIOProcClients++;

	return true;
}

void IOProcessor::Shutdown()
{
	numIOProcClients--;

	if (iocp == NULL || numIOProcClients > 0)
		return;

	delete[] iods;
	freeIods = NULL;
	CloseHandle(iocp);
	iocp = NULL;

}

static bool RequestReadNotification(IOOperation* ioop)
{
	DWORD	numBytes, flags;
	WSABUF	wsabuf;
	IODesc*	iod;
	int		ret;

//	Log_Trace("fd.index = %d", ioop->fd.index);

	iod = GetIODesc(ioop->fd);

	assert(iod->read == NULL);

	wsabuf.buf = NULL;
	wsabuf.len = 0;

	flags = 0;
	memset(&iod->ovlRead, 0, sizeof(OVERLAPPED));

	if (ioop->type == UDP_READ)
		flags |= MSG_PEEK;	// without this, the O/S would discard each incoming UDP packet
							// as it doesn't fit into the user-supplied (0 length) buffer

	ret = WSARecv(ioop->fd.sock, &wsabuf, 1, &numBytes, &flags, &iod->ovlRead, NULL);
	if (ret == SOCKET_ERROR)
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			Log_Trace("%d", ret);
			return false;
		}
	}

	iod->read = ioop;
	ioop->active = true;

	return true;
}

static bool RequestWriteNotification(IOOperation* ioop)
{
	DWORD	numBytes;
	WSABUF	wsabuf;
	IODesc*	iod;
	int		ret;

//	Log_Trace("fd.index = %d", ioop->fd.index);

	iod = GetIODesc(ioop->fd);

	assert(iod->write == NULL);

	wsabuf.buf = NULL;
	wsabuf.len = 0;

	memset(&iod->ovlWrite, 0, sizeof(OVERLAPPED));
	if (WSASend(ioop->fd.sock, &wsabuf, 1, &numBytes, 0, &iod->ovlWrite, NULL) == SOCKET_ERROR)
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			Log_Trace("ret = %d", ret);
			return false;
		}
	}

	iod->write = ioop;
	ioop->active = true;

	return true;
}

static bool StartAsyncAccept(IOOperation* ioop)
{
	DWORD	numBytes;
	BOOL	trueval = TRUE;
	IODesc*	iod;
	int		ret;

	iod = GetIODesc(ioop->fd);

	if (iod->read || iod->write)
		ASSERT_FAIL();

	// create an accepting socket with WSA_FLAG_OVERLAPPED to support async operations
	iod->acceptFd.sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (iod->acceptFd.sock == INVALID_SOCKET)
		return false;

	if (setsockopt(iod->acceptFd.sock, SOL_SOCKET, SO_REUSEADDR, (char *)&trueval, sizeof(BOOL)))
	{
		closesocket(iod->acceptFd.sock);
		return false;
	}

	memset(&iod->ovlRead, 0, sizeof(OVERLAPPED));
	if (!AcceptEx(ioop->fd.sock, iod->acceptFd.sock, iod->acceptData, 0, ACCEPT_ADDR_LEN, ACCEPT_ADDR_LEN, &numBytes, &iod->ovlRead))
	{
		ret = WSAGetLastError();
		if (ret != WSA_IO_PENDING)
		{
			Log_Errno();
			closesocket(iod->acceptFd.sock);
			return false;
		}
	}

	iod->read = ioop;
	ioop->active = true;

	return true;
}

bool StartAsyncConnect(IOOperation* ioop)
{
	IODesc*	iod;

	iod = GetIODesc(ioop->fd);
	iod->write = ioop;
	ioop->active = true;
	
	return true;
}

bool IOProcessor::Add(IOOperation* ioop)
{
	if (ioop->active)
	{
		ASSERT_FAIL();
//		return true;
	}

	// empty buffer indicates that we are waiting for accept event
	if (ioop->type == TCP_READ && ioop->data.buffer == NULL)
		return StartAsyncAccept(ioop);

	// zero length indicates that we are waiting for connect event
	if (ioop->type == TCP_WRITE && ioop->data.length == 0)
		return StartAsyncConnect(ioop);

	switch (ioop->type)
	{
	case TCP_READ:
	case UDP_READ:
		return RequestReadNotification(ioop);
		break;
	case TCP_WRITE:
	case UDP_WRITE:
		return RequestWriteNotification(ioop);
		break;
	}

	return false;
}

bool IOProcessor::Remove(IOOperation *ioop)
{
	int			ret;
	IODesc*		iod;
	TCPRead*	tcpread;

	if (ioop->type == TCP_READ)
	{
		tcpread = (TCPRead*) ioop;
		if (tcpread->listening)
			ASSERT_FAIL(); // Remove() not supported for listening sockets
	}

	ioop->active = false;
	iod = GetIODesc(ioop->fd);

	ret = CancelIo((HANDLE)ioop->fd.sock);
	if (ret == 0)
	{
		ret = WSAGetLastError();
		Log_Trace("ret = %d", ret);
		return false;
	}

	if (ioop->type == TCP_READ || ioop->type == UDP_READ)
	{
		iod->read = NULL;
		if (iod->write != NULL)
		{
			assert(iod->write->active);
			ioop = iod->write;
			iod->write = NULL;
			ioop->active = false;
			IOProcessor::Add(ioop);
		}
	}
	else
	{
		iod->write = NULL;
		if (iod->read != NULL)
		{
			assert(iod->read->active);
			ioop = iod->read;
			iod->read = NULL;
			ioop->active = false;
			IOProcessor::Add(ioop);
		}
	}

	return true;
}

bool IOProcessor::Poll(int msec)
{
	DWORD			numBytes;
	DWORD			timeout;
	OVERLAPPED*		overlapped;
	BOOL			ret;
	BOOL			result;
	DWORD			error;
	DWORD			flags;
	IODesc*			iod;
	IOOperation*	ioop;

	timeout = (msec >= 0) ? msec : INFINITE;

	ret = GetQueuedCompletionStatus(iocp, &numBytes, (PULONG_PTR)&iod, &overlapped, timeout);
	if (terminated)
		return false;

	EventLoop::UpdateTime();
	ioop = NULL;
	flags = 0;
	// ret == TRUE: a completion packet for a successful I/O operation was dequeued
	// ret == FALSE && overlapped != NULL: a completion packet for a failed I/O operation was dequeued
	if (ret || overlapped)
	{
		if (iod == &callback)
		{
			Call((Callable*) overlapped);
		}
		// sometimes we get this after closesocket, so check first
		// if iod is in the freelist
		// TODO clarify which circumstances lead to this issue
		else if (iod->next == NULL && (iod->read || iod->write))
		{
			if (overlapped == &iod->ovlRead && iod->read)
			{
				ioop = iod->read;
				assert(ioop->active);
				iod->read = NULL;

				result = WSAGetOverlappedResult(ioop->fd.sock, &iod->ovlRead, &numBytes, FALSE, &flags);

				if (ioop->type == UDP_READ)
					return ProcessUDPRead((UDPRead*)ioop);
				else if (result)
					return ProcessTCPRead((TCPRead*)ioop);
				else
				{
					error = GetLastError();
					if (error != WSA_IO_INCOMPLETE)
					{
						Log_Trace("last error = %d", error);
						ioop->active = false;
						Call(ioop->onClose);
					}

				}
			}
			else if (overlapped == &iod->ovlWrite && iod->write)
			{
				ioop = iod->write;
				assert(ioop->active);
				iod->write = NULL;

				result = WSAGetOverlappedResult(ioop->fd.sock, &iod->ovlWrite, &numBytes, FALSE, &flags);

				if (result && ioop->type == TCP_WRITE)
					return ProcessTCPWrite((TCPWrite*)ioop);
				else if (result && ioop->type == UDP_WRITE)
					return ProcessUDPWrite((UDPWrite*)ioop);
				else
				{
					error = GetLastError();
					if (error != WSA_IO_INCOMPLETE)
					{
						Log_Trace("last error = %d", error);
						ioop->active = false;
						Call(ioop->onClose);
					}
				}
			}
		}
		timeout = 0;
	}
	else
	{
		// no completion packet was dequeued
		error = GetLastError();
		if (error != WAIT_TIMEOUT)
			Log_Trace("last error = %d", error);

		return true;
	}

	return true;
}

bool IOProcessor::Complete(Callable *callable)
{
	BOOL	ret;
	DWORD	error;

	ret = PostQueuedCompletionStatus(iocp, 0, (ULONG_PTR) &callback, (LPOVERLAPPED) callable);
	if (!ret)
	{
		error = GetLastError();
		Log_Trace("error = %d", error);
		return false;
	}

	return true;
}

bool ProcessTCPRead(TCPRead* tcpread)
{
	BOOL		ret;
	DWORD		flags;
	DWORD		error;
	WSABUF		wsabuf;
	Callable*	callable;
	DWORD		numBytes;

	callable = NULL;

	if (tcpread->listening)
	{
		callable = tcpread->onComplete;
	}
	else
	{
		wsabuf.buf = (char*) tcpread->data.buffer + tcpread->data.length;
		wsabuf.len = tcpread->data.size - tcpread->data.length;
		if (wsabuf.len > MAX_TCP_READ)
			wsabuf.len = MAX_TCP_READ;

		flags = 0;
		ret = WSARecv(tcpread->fd.sock, &wsabuf, 1, &numBytes, &flags, NULL, NULL);
		if (ret != 0)
		{
			error = GetLastError();
			if (error == WSAEWOULDBLOCK)
			{
				tcpread->active = false; // otherwise Add() returns
				IOProcessor::Add(tcpread);
			}
			else
				callable = tcpread->onClose;
		}
		else if (numBytes == 0)
			callable = tcpread->onClose;
		else
		{
			tcpread->data.length += numBytes;
			assert(tcpread->data.length <= tcpread->data.size);
			callable = tcpread->onComplete;
		}
	}

	if (callable)
	{
		tcpread->active = false;
		Call(callable);
	}

	return true;
}

bool ProcessUDPRead(UDPRead* udpread)
{
	BOOL		ret;
	DWORD		flags;
	DWORD		error;
	WSABUF		wsabuf;
	Callable*	callable;
	DWORD		numBytes;

	callable = NULL;

	wsabuf.buf = (char*) udpread->data.buffer;
	wsabuf.len = udpread->data.size;

	flags = 0;
	ret = WSARecv(udpread->fd.sock, &wsabuf, 1, &numBytes, &flags, NULL, NULL);
	if (ret != 0)
	{
		error = GetLastError();
		if (error == WSAEWOULDBLOCK)
		{
			udpread->active = false; // otherwise Add() returns
			IOProcessor::Add(udpread);
		}
		else
			callable = udpread->onClose;
	}
	else if (numBytes == 0)
		callable = udpread->onClose;
	else
	{
		udpread->data.length = numBytes;
		assert(udpread->data.length <= udpread->data.size);
		callable = udpread->onComplete;
	}

	if (callable)
	{
		udpread->active = false;
		Call(callable);
	}

	return true;
}

bool ProcessTCPWrite(TCPWrite* tcpwrite)
{
	BOOL		ret;
	WSABUF		wsabuf;
	Callable*	callable;
	DWORD		numBytes;
	DWORD		error;

	callable = NULL;

	if (tcpwrite->data.length == 0)
		callable = tcpwrite->onComplete; // tcp connect case
	else
	{
		// handle tcp write partial writes
		wsabuf.buf = (char*) tcpwrite->data.buffer + tcpwrite->transferred;
		// the -1 is actually a windows bug, for more info see:
		// http://support.microsoft.com/kb/823764/EN-US/
		wsabuf.len = MIN(tcpwrite->data.length - tcpwrite->transferred, SEND_BUFFER_SIZE - 1);

		// perform non-blocking write
		ret = WSASend(tcpwrite->fd.sock, &wsabuf, 1, &numBytes, 0, NULL, NULL);
		if (ret != 0)
		{
			error = GetLastError();
			if (error == WSAEWOULDBLOCK)
			{
				tcpwrite->active = false; // otherwise Add() returns
				IOProcessor::Add(tcpwrite);
			}
			else
				callable = tcpwrite->onClose;
		}
		else if (numBytes == 0)
			callable = tcpwrite->onClose;
		else
		{
			tcpwrite->transferred += numBytes;
			if (tcpwrite->transferred == tcpwrite->data.length)
				callable = tcpwrite->onComplete;
			else
			{
				tcpwrite->active = false; // otherwise Add() returns
				IOProcessor::Add(tcpwrite);
			}
		}
	}

	if (callable)
	{
		tcpwrite->active = false;
		Call(callable);
	}

	return true;
}

bool ProcessUDPWrite(UDPWrite* ioop)
{
	ASSERT_FAIL();
	return false;
}

#endif
