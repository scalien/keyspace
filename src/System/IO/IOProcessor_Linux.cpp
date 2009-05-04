#include "System/Platform.h"
#ifdef PLATFORM_LINUX

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <fcntl.h>
#include "System/IO/IOProcessor.h"
#include "System/Common.h"
#include "System/Log.h"
#include "System/Time.h"

#define	MAX_EVENTS			1
#define PIPEOP				'p'


class PipeOp : public IOOperation
{
public:
	PipeOp()
	{
		type = PIPEOP;
		pipe[0] = pipe[1] = -1;
	}
	
	~PipeOp()
	{
		Close();
	}
	
	void Close()
	{
		if (pipe[0] >= 0)
		{
			close(pipe[0]);
			pipe[0] = -1;
		}
		if (pipe[1] >= 0)
		{
			close(pipe[1]);
			pipe[1] = -1;
		}
	}
	
	int			 	pipe[2];
	CFunc::Callback	callback;
};

class EpollOp
{
public:
	EpollOp()
	{
		read = NULL;
		write = NULL;
	}
	
	IOOperation*	read;
	IOOperation*	write;
};

static int			epollfd;
static int			maxfd;
static PipeOp		asyncPipeOp;
static EpollOp*		epollOps;

static bool			AddEvent(int fd, uint32_t filter, IOOperation* ioop);

static void			ProcessAsyncOp();
static void			ProcessIOOperation(IOOperation* ioop);
static void			ProcessTCPRead(TCPRead* tcpread);
static void			ProcessTCPWrite(TCPWrite* tcpwrite);
static void			ProcessUDPRead(UDPRead* udpread);
static void			ProcessUDPWrite(UDPWrite* udpwrite);


bool /*IOProcessor::*/InitPipe(PipeOp &pipeop, CFunc::Callback callback)
{
	if (pipe(pipeop.pipe) < 0)
	{
		Log_Errno();
		return false;
	}

	if (pipeop.pipe[0] < 0 || pipeop.pipe[1] < 0)
		return false;
	
	fcntl(pipeop.pipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(pipeop.pipe[0], F_SETFL, O_NONBLOCK);
	fcntl(pipeop.pipe[1], F_SETFL, O_NONBLOCK);

	if (!AddEvent(pipeop.pipe[0], EPOLLIN, &pipeop))
		return false;
	
	pipeop.callback = callback;

	return true;
}

bool IOProcessor::Init(int maxfd_)
{
	int i;
	rlimit rl;
	
	maxfd = maxfd_;
	rl.rlim_cur = maxfd;
	rl.rlim_max = maxfd;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		Log_Errno();
	}
	
	epollOps = new EpollOp[maxfd];
	for (i = 0; i < maxfd; i++)
	{
		epollOps[i].read = NULL;
		epollOps[i].write = NULL;
	}

	epollfd = epoll_create(maxfd);
	if (epollfd < 0)
	{
		Log_Errno();
		return false;
	}
	
	if (!InitPipe(asyncPipeOp, ProcessAsyncOp))
		return false;

	return true;
}

void IOProcessor::Shutdown()
{
	close(epollfd);
	asyncPipeOp.Close();
}

bool IOProcessor::Add(IOOperation* ioop)
{
	uint32_t	filter;
	
	filter = EPOLLONESHOT;
	if (ioop->type == TCP_READ || ioop->type == UDP_READ)
		filter |= EPOLLIN;
	else if (ioop->type == TCP_WRITE || ioop->type == UDP_WRITE)
		filter |= EPOLLOUT;
	
	return AddEvent(ioop->fd, filter, ioop);
}

bool AddEvent(int fd, uint32_t event, IOOperation* ioop)
{
	int					nev;
	struct epoll_event	ev;
	EpollOp				*epollOp;
	bool				hasEvent;
	
	if (epollfd < 0)
	{
		Log_Message("epollfd < 0");
		return false;
	}

	epollOp = &epollOps[fd];
	hasEvent = epollOp->read || epollOp->write;

	if ((event & EPOLLIN) == EPOLLIN)
		epollOp->read = ioop;
	if ((event & EPOLLOUT) == EPOLLOUT)
		epollOp->write = ioop;

	if (epollOp->read)
		event |= EPOLLIN;
	if (epollOp->write)
		event |= EPOLLOUT;

	ev.events = event;
	ev.data.ptr = epollOp;
	
	// add our interest in the event
	nev = epoll_ctl(epollfd, hasEvent ? EPOLL_CTL_MOD : EPOLL_CTL_ADD, fd, &ev);

    // If you add the same fd to an epoll_set twice, you
    // probably get EEXIST, but this is a harmless condition.
	if (nev < 0)
	{
		if (errno == EEXIST)
		{
			//Log_Message("AddEvent: fd = %d exists", fd);
			nev = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
		}

		if (nev < 0)
		{
			Log_Errno();
			return false;
		}
	}
	
	if (ioop)
		ioop->active = true;
	
	return true;
}

bool IOProcessor::Remove(IOOperation* ioop)
{
	int					nev;
	struct epoll_event	ev;
	EpollOp*			epollOp;
	
	if (epollfd < 0)
	{
		Log_Message("eventfd < 0");
		return false;
	}
	
	epollOp = &epollOps[ioop->fd];	
	if (ioop->type == TCP_READ || ioop->type == UDP_READ)
	{
		epollOp->read = NULL;
		if (epollOp->write)
			ev.events = EPOLLOUT;
	}
	else
	{
		epollOp->write = NULL;
		if (epollOp->read)
			ev.events = EPOLLIN;
	}

	ev.events |= EPOLLONESHOT;
	ev.data.ptr = ioop;

	if (epollOp->read || epollOp->write)
		nev = epoll_ctl(epollfd, EPOLL_CTL_MOD, ioop->fd, &ev);
	else
		nev = epoll_ctl(epollfd, EPOLL_CTL_DEL, ioop->fd, &ev /* this paramter is ignored */);

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
	
	long long					called;
	int							i, nevents, nev, wait;
	static struct epoll_event	events[MAX_EVENTS];
	IOOperation*				ioop;
	EpollOp*					epollOp;
	int							newev, currentev;
	int							newfd = -1;
	
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
			currentev = events[i].events;
			
			epollOp = (EpollOp*) events[i].data.ptr;

			newev = 0;
			if ((events[i].events & EPOLLIN) && epollOp->write)
			{
				newev |= EPOLLOUT;
				newfd = epollOp->write->fd;
			}
			else if ((events[i].events & EPOLLOUT) && epollOp->read)
			{
				newev |= EPOLLIN;
				newfd = epollOp->read->fd;
			}
			
			if (newev)
			{
				if ((epollOp->read &&
					(epollOp->read->type == TCP_READ || epollOp->read->type == UDP_READ)) ||
					(epollOp->write &&
					(epollOp->write->type == TCP_WRITE || epollOp->write->type == UDP_WRITE)))
						newev |= EPOLLONESHOT;

				events[i].events = newev;
				nev = epoll_ctl(epollfd, EPOLL_CTL_MOD, newfd, &events[i]);
				if (nev < 0)
				{
					Log_Errno();
					return false;
				}
			}
			
			if (currentev & EPOLLIN)
			{
				ioop = epollOp->read;
				assert(ioop != NULL);
								
				if (ioop->type == PIPEOP)
				{
					PipeOp* pipeop = (PipeOp*) ioop;
					pipeop->callback();
					continue;
				}
				
				epollOp->read = NULL;
				ProcessIOOperation(ioop);
			}
			else if (currentev & EPOLLOUT)
			{
				ioop = epollOp->write;
				assert(ioop != NULL);
				epollOp->write = NULL;
				ProcessIOOperation(ioop);
			}
		}
			
	} while (nevents > 0 && wait > 0);
	
	return true;
}

bool IOProcessor::Complete(Callable* callable)
{
	Log_Trace();
	
	int nwrite;
	
	nwrite = write(asyncPipeOp.pipe[1], &callable, sizeof(Callable*));
	if (nwrite < 0)
	{
		Log_Errno();
		return false;
	}
	
	return true;
}

void ProcessIOOperation(IOOperation* ioop)
{
	ioop->active = false;
	
	if (ioop && ioop->type == TCP_READ)
		ProcessTCPRead((TCPRead*) ioop);
	else if (ioop && ioop->type == TCP_WRITE)
		ProcessTCPWrite((TCPWrite*) ioop);
	else if (ioop && ioop->type == UDP_READ)
		ProcessUDPRead((UDPRead*) ioop);
	else if (ioop && ioop->type == UDP_WRITE)
		ProcessUDPWrite((UDPWrite*) ioop);
}

#define MAX_CALLABLE 256	
void ProcessAsyncOp()
{
	Log_Trace();

	Callable* callables[MAX_CALLABLE];
	int nread;
	int count;
	int i;
	
	while (true)
	{
		nread = read(asyncPipeOp.pipe[0], callables, SIZE(callables));
		count = nread / sizeof(Callable*);
		
		for (i = 0; i < count; i++)
			Call(callables[i]);
		
		if (count < (int) SIZE(callables))
			break;
	}
}

void ProcessTCPRead(TCPRead* tcpread)
{
	int			readlen, nread;

	if (tcpread->listening)
	{
		Call(tcpread->onComplete);
		return;
	}
	
	if (tcpread->requested == IO_READ_ANY)
		readlen = tcpread->data.size - tcpread->data.length;
	else
		readlen = tcpread->requested - tcpread->data.length;
	
	if (readlen <= 0)
		return;

	nread = read(tcpread->fd, tcpread->data.buffer + tcpread->data.length, readlen);
	if (nread < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			IOProcessor::Add(tcpread);
		else
		{
			Log_Errno();
			Call(tcpread->onClose);
		}
	}
	else if (nread == 0)
	{
		Call(tcpread->onClose);
	}
	else
	{
		tcpread->data.length += nread;
		if (tcpread->requested == IO_READ_ANY || 
			tcpread->data.length == (unsigned)tcpread->requested)
			Call(tcpread->onComplete);
		else
			IOProcessor::Add(tcpread);
	}
}

void ProcessTCPWrite(TCPWrite* tcpwrite)
{
	int			writelen, nwrite;

	// this indicates check for connect() readyness
	if (tcpwrite->data.length == 0)
	{
		sockaddr_in sa;
		socklen_t	socklen;
		
		nwrite = getpeername(tcpwrite->fd, (sockaddr*) &sa, &socklen);
		if (nwrite < 0)
			Call(tcpwrite->onClose);
		else
			Call(tcpwrite->onComplete);
	
		return;
	}

	writelen = tcpwrite->data.length - tcpwrite->transferred;
	if (writelen <= 0)
	{
		ASSERT_FAIL();
	}
	
	nwrite = write(tcpwrite->fd, tcpwrite->data.buffer, writelen);
	if (nwrite < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			IOProcessor::Add(tcpwrite);
		else
		{
			Log_Errno();
			Call(tcpwrite->onClose);
		}
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
			IOProcessor::Add(tcpwrite);
	}
}

void ProcessUDPRead(UDPRead* udpread)
{
	int			salen, nread;
	
	salen = sizeof(udpread->endpoint.sa);

	do {
		nread = recvfrom(udpread->fd, udpread->data.buffer, udpread->data.size, 0,
				(sockaddr*)&udpread->endpoint.sa, (socklen_t*)&salen);
	
		if (nread < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				IOProcessor::Add(udpread); // try again
			else
			{
				Log_Errno();
				Call(udpread->onClose);
			}
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
	} while (nread > 0);
}

void ProcessUDPWrite(UDPWrite* udpwrite)
{
	int			nwrite;

	nwrite = sendto(udpwrite->fd, udpwrite->data.buffer + udpwrite->offset, udpwrite->data.length - udpwrite->offset, 0,
				(const sockaddr*)&udpwrite->endpoint.sa, sizeof(udpwrite->endpoint.sa));

	if (nwrite < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			IOProcessor::Add(udpwrite); // try again
		else
		{
			Log_Errno();
			Call(udpwrite->onClose);
		}
	}
	else if (nwrite == 0)
	{
		Call(udpwrite->onClose);
	}
	else
	{
		if (nwrite == (int)udpwrite->data.length - udpwrite->offset)
		{
			Call(udpwrite->onComplete);
		} else
		{
			udpwrite->offset += nwrite;
			Log_Message("sendto() datagram fragmentation");
			IOProcessor::Add(udpwrite); // try again
		}
	}
}

#endif // PLATFORM_LINUX
