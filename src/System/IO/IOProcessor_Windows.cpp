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

// this structure is used for storing Windows/IOCP specific data
struct IODesc
{
	IOOperation*	read;
	IOOperation*	write;

	Callable*		readCallback;
	OVERLAPPED		ovlRead;			// for accept/recv
	OVERLAPPED		ovlWrite;			// for connect/send

	FD				acceptFd;			// with a listening socket, the accepted FD is stored here
	byte			acceptData[2 * ACCEPT_ADDR_LEN];	// TODO allocate dynamically

	IODesc*			next;				// pointer for free list handling
};

static HANDLE	iocp;					// global completion port handle
static IODesc*	iods;					// pointer to allocated array of IODesc's
static IODesc*	freeIods;				// pointer to the free list of IODesc's
static IODesc	callback;				// special IODesc for handling IOProcessor::Complete events
const FD		INVALID_FD = {-1, INVALID_SOCKET};	// special FD to indicate invalid value

static LPFN_CONNECTEX	ConnectEx;

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

	iod = freeIods;
	// unlink iod from free list
	freeIods = iod->next;
	iod->next = NULL;
	iod->readCallback = NULL;
	iod->read = NULL;
	iod->write = NULL;

	fd.index = iod - iods;

	CreateIoCompletionPort((HANDLE)fd.sock, iocp, (ULONG_PTR)iod, 0);
		
	return true;
}

// put back the IODesc to the free list and cancel all io and put back FD
bool IOProcessorUnregisterSocket(FD& fd)
{
	IODesc*		iod;

	iod = &iods[fd.index];
	iod->next = freeIods;
	freeIods = iod;

	CancelIo((HANDLE)fd.sock);

	return true;
}

bool IOProcessor::Init(int maxfd)
{
	WSADATA		wsaData;
	SOCKET		s;
	GUID		guid = WSAID_CONNECTEX;
	DWORD		bytesReturned;

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
	{
		iods[i].next = &iods[i + 1];
	}
	iods[maxfd - 1].next = NULL;
	freeIods = iods;

	return true;
}

void IOProcessor::Shutdown()
{
	CloseHandle(iocp);
	WSACleanup();
}

static bool StartAsyncRead(IOOperation* ioop)
{
	DWORD	numBytes, flags;
	WSABUF	wsabuf;
	IODesc*	iod;
	int		ret;

	Log_Trace("fd.index = %d", ioop->fd.index);

	iod = GetIODesc(ioop->fd);

	// if there is an active callback, complete it before the new async operation
	if (iod->readCallback)
	{
		PostQueuedCompletionStatus(iocp, 0, (ULONG_PTR) &iod, NULL);
		ioop->active = true;
		return true;
	}

	if (iod->read)
		return false;

	wsabuf.buf = (char *)ioop->data.buffer + ioop->data.length;
	wsabuf.len = ioop->data.size - ioop->data.length;

	flags = 0;
	memset(&iod->ovlRead, 0, sizeof(OVERLAPPED));
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

static bool StartAsyncWrite(IOOperation* ioop)
{
	DWORD	numBytes;
	WSABUF	wsabuf;
	IODesc*	iod;
	int		ret;

	Log_Trace("fd.index = %d", ioop->fd.index);

	iod = GetIODesc(ioop->fd);

	if (iod->write)
		return false;

	wsabuf.buf = (char *)ioop->data.buffer;
	wsabuf.len = ioop->data.length;

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
		return false;

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
		return false;

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
		return StartAsyncRead(ioop);
		break;
	case TCP_WRITE:
	case UDP_WRITE:
		return StartAsyncWrite(ioop);
		break;
	}

	return false;
}

bool IOProcessor::Remove(IOOperation *ioop)
{
	IODesc*	iod;

	ioop->active = false;
	iod = GetIODesc(ioop->fd);

	switch (ioop->type)
	{
	case TCP_READ:
	case UDP_READ:
		/*
		 * Here the ioop's active flag is set to false. This indicates
		 * for Poll that there is no need for calling the callback.
		 * Next time the ioop is Added, the callback is activated.
		 */
		break;
	case TCP_WRITE:
	case UDP_WRITE:
		iod->write = NULL;
		break;
	}

	return true;
}

bool IOProcessor::Poll(int msec)
{
	DWORD		numBytes;
	DWORD		timeout;
	OVERLAPPED*	overlapped;
	BOOL		ret;
	DWORD		error;
	DWORD		flags;
	IODesc*		iod;
	Callable*	callable;

	//Log_Trace("msec = %d", msec);

	timeout = (msec >= 0) ? msec : INFINITE;

	ret = GetQueuedCompletionStatus(iocp, &numBytes, (PULONG_PTR)&iod, &overlapped, timeout);
	EventLoop::UpdateTime();

	// ret == TRUE: a completion packet for a successful I/O operation was dequeued
	// ret == FALSE && overlapped != NULL: a completion packet for a failed I/O operation was dequeued
	if (ret || overlapped)
	{
		if (iod == &callback)
		{
			callable = (Callable *) overlapped;
			Call(callable);
		}
		else if (iod->readCallback)
		{
			if (iod->read && iod->read->active)
			{
				iod->read->active = false;
				iod->read = NULL;
				callable = iod->readCallback;
				iod->readCallback = NULL;
				Call(callable);
			}
		}
		// sometimes we get this after closesocket, so check first
		// if iod is in the freelist
		// TODO clarify which circumstances lead to this issue
		else if (iod->next == NULL && (iod->read || iod->write))
		{
			if (overlapped == &iod->ovlRead && iod->read)
			{
				Log_Trace("read: %d, data.size: %d", numBytes, iod->read->data.size);
				if (iod->read->type == UDP_READ)
					iod->read->data.length = numBytes;
				else
					iod->read->data.length += numBytes;
				assert(iod->read->data.length <= iod->read->data.size);

				if ((iod->read->data.size > 0 && numBytes > 0) ||
					(iod->read->data.size == 0 && numBytes == 0))
				{
					callable = iod->read->onComplete;
				}
				else
					callable = iod->read->onClose;

				if (iod->read->active)
				{
					iod->read->active = false;
					iod->read = NULL;
					Call(callable);
				}
				else
				{
					iod->read = NULL;
					iod->readCallback = callable;
				}
			}
			else if (overlapped == &iod->ovlWrite && iod->write)
			{
				Log_Trace("write: %d", numBytes);
				
				if (numBytes == iod->write->data.length)
				{
					// check connection status
					if (iod->write->data.length == 0)
					{
						ret = WSAGetOverlappedResult(iod->write->fd.sock, &iod->ovlWrite, &numBytes, FALSE, &flags);
						if (ret)
							callable = iod->write->onComplete;
						else
							callable = iod->write->onClose;
					}
					else
						callable = iod->write->onComplete;
				}
				else
					callable = iod->write->onClose;

				iod->write->active = false;
				iod->write = NULL;
				Call(callable);
			}
		}
		timeout = 0;
	}
	else
	{
		// no completion packet was dequeued
		// TODO: check for errors
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

#endif
