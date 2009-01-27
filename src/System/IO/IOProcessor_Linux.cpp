#include "System/Platform.h"
#ifdef PLATFORM_LINUX

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include "System/IO/IOProcessor.h"
#include "System/Common.h"
#include "System/Log.h"
#include "System/Time.h"

#define EPOLL_EVENT_SIZE	1024
#define	MAX_EVENTS			256

// this is the singleton object
static IOProcessor	ioproc;

static int			epollfd;
static int			asyncPipe[2];

static bool			AddEvent(int fd, uint32_t filter, IOOperation* ioop);
static bool			AddAio(FileOp* ioop);

static void			ProcessAsyncEvent();
static void			ProcessFileCompletion(FileOp *ioop);
static void			ProcessTCPRead(struct epoll_event* ev);
static void			ProcessTCPWrite(struct epoll_event* ev);
static void			ProcessUDPRead(struct epoll_event* ev);
static void			ProcessUDPWrite(struct epoll_event* ev);

IOProcessor* IOProcessor::New()
{
	return &ioproc;
}

static void IOProc_sigev_thread_handler(union sigval value)
{
	FileOp *fileop = (FileOp *) value.sival_ptr;
	ssize_t ret;

	ret = write(asyncPipe[1], &fileop, sizeof(FileOp *));
	
	Log_Message("ret = %d", ret);
}

bool IOProcessor::Init()
{
	epollfd = epoll_create(EPOLL_EVENT_SIZE);
	if (epollfd < 0)
	{
		Log_Errno();
		return false;
	}
	
	if (pipe(asyncPipe) < 0)
	{
		Log_Errno();
		return false;
	}

	Log_Message("asyncPipe[0] = %d, asyncPipe[1] = %d", asyncPipe[0], asyncPipe[1]);
	if (asyncPipe[0] < 0 || asyncPipe[1] < 0)
		return false;
	
	fcntl(asyncPipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(asyncPipe[0], F_SETFL, O_NONBLOCK);
	
	fcntl(asyncPipe[1], F_SETFD, FD_CLOEXEC);
	fcntl(asyncPipe[1], F_SETFL, O_NONBLOCK);

	AddEvent(asyncPipe[0], EPOLLIN | EPOLLOUT, NULL);
	
	return true;
}

void IOProcessor::Shutdown()
{
	close(epollfd);
	close(asyncPipe[0]);
	close(asyncPipe[1]);
}

bool IOProcessor::Add(IOOperation* ioop)
{
	uint32_t	filter;
	
	if (ioop->type == FILE_READ || ioop->type == FILE_WRITE)
	{
		return AddAio((FileOp*) ioop);
	}
	else
	{
		if (ioop->type == TCP_READ || ioop->type == UDP_READ)
			filter = EPOLLET | EPOLLIN;
		else
			filter = EPOLLET | EPOLLOUT;
		
		return AddEvent(ioop->fd, filter, ioop);
	}
}

bool AddAio(FileOp* fileop)
{
	memset(&(fileop->cb), 0, sizeof(struct aiocb));
	
	fileop->cb.aio_fildes	= fileop->fd;
	fileop->cb.aio_offset	= fileop->offset;
	fileop->cb.aio_buf		= fileop->data.buffer;
	fileop->cb.aio_nbytes	= fileop->nbytes;
		
	fileop->cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
	fileop->cb.aio_sigevent.sigev_value.sival_ptr = fileop;
	fileop->cb.aio_sigevent.sigev_notify_attributes = NULL;
	fileop->cb.aio_sigevent.sigev_notify_function = IOProc_sigev_thread_handler;
	Log_Message("fileop = %p\n", fileop->cb.aio_sigevent.sigev_value.sival_ptr);

	if (fileop->type == FILE_READ)
	{
		if (aio_read(&(fileop->cb)) < 0)
		{
			Log_Errno();
			return false;
		}
	} else
	{
		if (aio_write(&(fileop->cb)) < 0)
		{
			Log_Errno();
			return false;
		}
	}
	
	fileop->active = true;
	return true;
}

bool AddEvent(int fd, uint32_t event, IOOperation* ioop)
{
	int				nev;
	struct epoll_event	ev;
	
	if (epollfd < 0)
	{
		Log_Message("epollfd < 0");
		return false;
	}

	ev.events = event;
	ev.data.ptr = ioop;
	
	// add our interest in the event
	nev = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
	if (nev < 0)
	{
		Log_Errno();
		return false;
	}
	
	if (ioop)
		ioop->active = true;
	
	return true;
}

bool IOProcessor::Remove(IOOperation* ioop)
{
	int				nev;
	struct epoll_event	ev;
	
	if (epollfd < 0)
	{
		Log_Message("eventfd < 0");
		return false;
	}
	
	if (ioop->type == TCP_READ || ioop->type == UDP_READ)
		ev.events = EPOLLIN;
	else
		ev.events = EPOLLOUT;

	ev.data.fd = ioop->fd;
	
	// delete event
	nev = epoll_ctl(epollfd, EPOLL_CTL_DEL, ioop->fd, &ev);
	if (nev < 0)
	{
		Log_Errno();
		return false;
	}
	
	ioop->active = false;
	
	return true;
}

bool IOProcessor::Poll(int sleep)
{
	
	long long				called;
	int						i, nevents, wait;
	static struct epoll_event	events[MAX_EVENTS];
	IOOperation*					ioop;
	
	called = Now();
	
	do
	{
		wait = sleep - (Now() - called);
		if (wait < 0) wait = 0;
		
		nevents = epoll_wait(epollfd, events, SIZE(events), wait);
		
		if (nevents < 0)
		{
			Log_Errno();
			return false;
		}
		
		for (i = 0; i < nevents; i++)
		{
			if (events[i].data.ptr == NULL)
			{
				ProcessAsyncEvent();
				continue;
			}

			ioop = (IOOperation *) events[i].data.ptr;
			
			ioop->active = false;
			
			
			if (ioop && ioop->type == TCP_READ && (events[i].events & EPOLLIN))
				ProcessTCPRead(&events[i]);
			
			if (ioop && ioop->type == TCP_WRITE && (events[i].events & EPOLLOUT))
				ProcessTCPWrite(&events[i]);
			
			if (ioop && ioop->type == UDP_READ && (events[i].events & EPOLLIN))
				ProcessUDPRead(&events[i]);
			
			if (ioop && ioop->type == UDP_WRITE && (events[i].events & EPOLLOUT))
				ProcessUDPWrite(&events[i]);
		}
	} while (nevents > 0 && wait > 0);
	
	return true;
}

void ProcessAsyncEvent()
{
	int	size;
	FileOp	*ops[256];
	FileOp	*fileop;
	int	pipefd;
	int	numBytes;
	int	count;
	int	i;

	Log_Message("");
	
	while (1)
	{
		pipefd = asyncPipe[0];
		size = read(pipefd, ops, sizeof(ops) * sizeof(FileOp *));
		count = size / sizeof(FileOp *);
		
		for (i = 0; i < count; i++)
		{
			fileop = ops[i];
			numBytes = aio_return(&fileop->cb);
			if (numBytes == EINPROGRESS)
				continue;
			
			ProcessFileCompletion(fileop);
		}
		
		if ((size_t) size < sizeof(ops))
			break;
	}
}

void ProcessFileCompletion(FileOp *fileop)
{
	Log_Message("fileop = %p\n", fileop);
	Call(fileop->onComplete);
}


void ProcessTCPRead(struct epoll_event* ev)
{
	int			readlen, nread;
	TCPRead*	tcpread;
	
	tcpread = (TCPRead*) ev->data.ptr;

	if (tcpread->listening)
	{
		Call(tcpread->onComplete);
	}
	else
	{
		if (tcpread->requested == IO_READ_ANY)
			readlen = tcpread->data.size - tcpread->data.length;
		else
			readlen = tcpread->requested - tcpread->data.length;
		if (readlen > 0)
		{
			//Log_Message(rprintf("Calling read() to read %d bytes", readlen));
			nread = read(tcpread->fd, tcpread->data.buffer + tcpread->data.length, readlen);
			if (nread < 0)
			{
				if (errno == EWOULDBLOCK || errno == EAGAIN)
					ioproc.Add(tcpread);
				else
					Log_Errno();
			}
			else if (nread == 0)
			{
				Call(tcpread->onClose);
			}
			else
			{
				tcpread->data.length += nread;
				if (tcpread->requested == IO_READ_ANY || tcpread->data.length == tcpread->requested)
					Call(tcpread->onComplete);
				else
					ioproc.Add(tcpread);
			}
		}
	}
}

void ProcessTCPWrite(struct epoll_event* ev)
{
	int			writelen, nwrite;
	TCPWrite*	tcpwrite;
	
	tcpwrite = (TCPWrite*) ev->data.ptr;

	writelen = tcpwrite->data.length - tcpwrite->transferred;
	// TODO writelen = min(ev->data, writelen);
	if (writelen > 0)
	{
		//Log_Message(rprintf("Calling write() to write %d bytes", writelen));
		nwrite = write(tcpwrite->fd, tcpwrite->data.buffer, writelen);
		if (nwrite < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				ioproc.Add(tcpwrite);
			else
				Log_Errno();
		} 
		else if (nwrite == 0)
		{
			Call(tcpwrite->onClose);
		}
		else
		{
			tcpwrite->transferred += nwrite;
			if (tcpwrite->transferred == tcpwrite->data.length)
				Call(tcpwrite->onComplete);
			else
				ioproc.Add(tcpwrite);
		}
	}

}

void ProcessUDPRead(struct epoll_event* ev)
{
	int			salen, nread;
	UDPRead*	udpread;

	udpread = (UDPRead*) ev->data.ptr;
	
	//Log_Message(rprintf("Calling recvfrom() to read max %d bytes", udpread->size));
	salen = sizeof(udpread->endpoint.sa);
	nread = recvfrom(udpread->fd, udpread->data.buffer, udpread->data.size, 0,
				(sockaddr*)&udpread->endpoint.sa, (socklen_t*)&salen);
	if (nread < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			ioproc.Add(udpread); // try again
		else
			Log_Errno();
	} 
	else if (nread == 0)
	{
		Call(udpread->onClose);
	}
	else
	{
		udpread->data.length = nread;
		Call(udpread->onComplete);
	}
}

void ProcessUDPWrite(struct epoll_event* ev)
{
	int			nwrite;
	UDPWrite*	udpwrite;

	udpwrite = (UDPWrite*) ev->data.ptr;

	//Log_Message(rprintf("Calling sendto() to write %d bytes", udpwrite->data));
	nwrite = sendto(udpwrite->fd, udpwrite->data.buffer + udpwrite->offset, udpwrite->data.length - udpwrite->offset, 0,
				(const sockaddr*)&udpwrite->endpoint.sa, sizeof(udpwrite->endpoint.sa));
	if (nwrite < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			ioproc.Add(udpwrite); // try again
		else
			Log_Errno();
	}
	else if (nwrite == 0)
	{
		Call(udpwrite->onClose);
	}
	else
	{
		if (nwrite == udpwrite->data.length - udpwrite->offset)
		{
			Call(udpwrite->onComplete);
		} else
		{
			udpwrite->offset += nwrite;
			Log_Message("sendto() datagram fragmentation");
			ioproc.Add(udpwrite); // try again
		}
	}
}

#endif // PLATFORM_LINUX
