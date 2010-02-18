#ifdef PLATFORM_DARWIN

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

#include "IOProcessor.h"
#include "System/Containers/List.h"
#include "System/Common.h"
#include "System/Log.h"
#include "System/Time.h"
#include "System/Events/EventLoop.h"

// see http://wiki.netbsd.se/index.php/kqueue_tutorial

#define	MAX_KEVENTS 1024

static int				kq = 0;			// the kqueue
static int				asyncOpPipe[2];
static volatile bool	terminated;

static bool AddKq(int ident, short filter, IOOperation* ioop);

static void ProcessAsyncOp();
static void ProcessTCPRead(struct kevent* ev);
static void ProcessTCPWrite(struct kevent* ev);
static void ProcessUDPRead(struct kevent* ev);
static void ProcessUDPWrite(struct kevent* ev);

void SignalHandler(int )
{
	terminated = true;
}

void SetupSignals()
{
	struct sigaction	sa;
	sigset_t			mask;
	
	sigfillset(&mask);
	pthread_sigmask(SIG_SETMASK, &mask, NULL);

	memset(&sa, 0, sizeof(sa));
	sigfillset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	
	sa.sa_handler = SignalHandler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGSYS, &sa, NULL);
	sigaction(SIGXCPU, &sa, NULL);
	sigaction(SIGXFSZ, &sa, NULL);

	sigemptyset(&mask);
	pthread_sigmask(SIG_SETMASK, &mask, NULL);

//	AddKq(SIGINT, EVFILT_SIGNAL, NULL);
//	AddKq(SIGTERM, EVFILT_SIGNAL, NULL);
//	AddKq(SIGBUS, EVFILT_SIGNAL, NULL);
//	AddKq(SIGINT, EVFILT_SIGNAL, NULL);
//	AddKq(SIGFPE, EVFILT_SIGNAL, NULL);
//	AddKq(SIGILL, EVFILT_SIGNAL, NULL);
//	AddKq(SIGSEGV, EVFILT_SIGNAL, NULL);
//	AddKq(SIGSYS, EVFILT_SIGNAL, NULL);
//	AddKq(SIGXCPU, EVFILT_SIGNAL, NULL);
//	AddKq(SIGXFSZ, EVFILT_SIGNAL, NULL);
}

bool IOProcessor::Init(int maxfd_)
{
	rlimit rl;

	if (kq != 0)
		return true;

	terminated = false;
	
	kq = kqueue();
	if (kq < 0)
	{
		Log_Errno();
		return false;
	}

	SetupSignals();	
	
	rl.rlim_cur = maxfd_;
	rl.rlim_max = maxfd_;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		Log_Errno();
	}
		
	// setup async pipe
	if (pipe(asyncOpPipe) < 0)
	{
		Log_Errno();
		return false;
	}
	
	fcntl(asyncOpPipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(asyncOpPipe[0], F_SETFD, O_NONBLOCK);
	fcntl(asyncOpPipe[1], F_SETFD, FD_CLOEXEC);

	if (!AddKq(asyncOpPipe[0], EVFILT_READ, NULL))
		return false;
		
	return true;
}

void IOProcessor::Shutdown()
{
	close(kq);
	kq = 0;
	close(asyncOpPipe[0]);
	close(asyncOpPipe[1]);
}

bool IOProcessor::Add(IOOperation* ioop)
{
	short	filter;
	
	if (ioop->active)
		return true;
	
	if (ioop->pending)
		return true;
	
	if (ioop->type == TCP_READ || ioop->type == UDP_READ)
		filter = EVFILT_READ;
	else
		filter = EVFILT_WRITE;
	
	return AddKq(ioop->fd, filter, ioop);
}

bool AddKq(int ident, short filter, IOOperation* ioop)
{
	int				nev;
	struct kevent	ev;
	struct timespec timeout = { 0, 0 };
	
	if (kq < 0)
	{
		Log_Trace("kq < 0");
		return false;
	}
	
	EV_SET(&ev, ident, filter, EV_ADD | EV_ONESHOT, 0, 0, ioop);
	
	// add our interest in the event
	nev = kevent(kq, &ev, 1, NULL, 0, &timeout);
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
	short			filter;
	int				nev;
	struct kevent	ev;
	struct timespec timeout = { 0, 0 };
	
	if (!ioop->active)
		return true;
		
	if (ioop->pending)
	{
		ioop->active = false;
		return true;
	}
	
	if (kq < 0)
	{
		Log_Trace("kq < 0");
		return false;
	}

	if (ioop->type == TCP_READ || ioop->type == UDP_READ)
		filter = EVFILT_READ;
	else
		filter = EVFILT_WRITE;
	
	EV_SET(&ev, ioop->fd, filter, EV_DELETE, 0, 0, 0);
	
	// delete event
	nev = kevent(kq, &ev, 1, NULL, 0, &timeout);
	if (nev < 0)
	{
		Log_Errno();
		// HACK:
		ioop->active = false;
		return false;
	}
	
	ioop->active = false;
	
	return true;
}

bool IOProcessor::Poll(int sleep)
{
	int						i, nevents;
	static struct kevent	events[MAX_KEVENTS];
	struct timespec			timeout;
	IOOperation*			ioop;
	
	timeout.tv_sec = (time_t) floor(sleep / 1000.0);
	timeout.tv_nsec = (sleep - 1000 * timeout.tv_sec) * 1000000;
	
	nevents = kevent(kq, NULL, 0, events, SIZE(events), &timeout);
	EventLoop::UpdateTime();

	if (nevents < 0 || terminated)
	{
		Log_Errno();
		Log_Trace("terminated = %s", terminated ? "true" : "false");
		if (terminated)
			return false;
		else
			return true;
	}
	
	for (i = 0; i < nevents; i++)
	{
		if (events[i].flags & EV_ERROR)
			Log_Trace("%s", strerror(events[i].data));
		if (events[i].udata == NULL)
			continue;
		ioop = (IOOperation*) events[i].udata;
		ioop->pending = true;
	}
	
	for (i = 0; i < nevents; i++)
	{
		if (events[i].filter == EVFILT_SIGNAL)
		{
			Log_Trace("SIGNAL caught, %d", events[i].ident);
			return false;
		}

		if (events[i].flags & EV_ERROR)
			continue;
		
		if (events[i].udata == NULL)
		{
			ProcessAsyncOp();
			// re-register for notification
			if (!AddKq(asyncOpPipe[0], EVFILT_READ, NULL))
				return false;
			continue;
		}

		ioop = (IOOperation*) events[i].udata;
		ioop->pending = false;
		if (!ioop->active)
			continue; // ioop was removed, just skip it			
		ioop->active = false;
		
		if (ioop->type == TCP_READ && (events[i].filter & EVFILT_READ))
			ProcessTCPRead(&events[i]);
		else if (ioop->type == TCP_WRITE && (events[i].filter & EVFILT_WRITE))
			ProcessTCPWrite(&events[i]);
		else if (ioop->type == UDP_READ && (events[i].filter & EVFILT_READ))
			ProcessUDPRead(&events[i]);
		else if (ioop->type == UDP_WRITE && (events[i].filter & EVFILT_WRITE))
			ProcessUDPWrite(&events[i]);
	}
	
	return true;
}

bool IOProcessor::Complete(Callable* callable)
{
	Log_Trace();

	int nwrite;
	
	nwrite = write(asyncOpPipe[1], &callable, sizeof(Callable*));
	if (nwrite < 0)
		Log_Errno();
	
	if (nwrite >= 0)
		return true;
	
	return false;
}

void ProcessAsyncOp()
{
	Log_Trace();
	
	static Callable	*callables[256];
	int nread;
	int count;
	int i;
	
	while (1)
	{
		nread = read(asyncOpPipe[0], callables, SIZE(callables));
		count = nread / sizeof(Callable*);
		
		for (i = 0; i < count; i++)
			Call(callables[i]);
		
		if (count < (int) SIZE(callables))
			break;
	}
}

void ProcessTCPRead(struct kevent* ev)
{
	int			readlen, nread;
	TCPRead*	tcpread;
	
	tcpread = (TCPRead*) ev->udata;

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
		readlen = MIN(ev->data, readlen);
		if (readlen > 0)
		{
			nread = read(tcpread->fd,
						 tcpread->data.buffer + tcpread->data.length,
						 readlen);
			
			if (nread < 0)
			{
				if (errno == EWOULDBLOCK || errno == EAGAIN)
					IOProcessor::Add(tcpread);
				else if (!(ev->flags & EV_EOF))
					Log_Errno();
			}
			else
			{
				tcpread->data.length += nread;
				if (tcpread->requested == IO_READ_ANY ||
				(int) tcpread->data.length == tcpread->requested)
					Call(tcpread->onComplete);
				else
					IOProcessor::Add(tcpread);
			}
		}
		
		if (ev->flags & EV_EOF)
			Call(tcpread->onClose);
	}
}

void ProcessTCPWrite(struct kevent* ev)
{
	int			writelen, nwrite;
	TCPWrite*	tcpwrite;
	
	tcpwrite = (TCPWrite*) ev->udata;
	
	if (ev->flags & EV_EOF)
	{
		Call(tcpwrite->onClose);
		return;
	}
	
	if (tcpwrite->data.length == 0)
	{
		if (ev->flags & EV_EOF)
			Call(tcpwrite->onClose);
		else
			Call(tcpwrite->onComplete);

		return;
	}

	writelen = tcpwrite->data.length - tcpwrite->transferred;
	writelen = MIN(ev->data, writelen);
	
	if (writelen > 0)
	{
		nwrite = write(tcpwrite->fd,
					   tcpwrite->data.buffer + tcpwrite->transferred,
					   writelen);
		
		if (nwrite < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				IOProcessor::Add(tcpwrite);
			else if (!(ev->flags & EV_EOF))
				Log_Errno();
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
}

void ProcessUDPRead(struct kevent* ev)
{
	int			salen, nread;
	UDPRead*	udpread;

	udpread = (UDPRead*) ev->udata;
	
	salen = ENDPOINT_SOCKADDR_SIZE;
	nread = recvfrom(udpread->fd,
					 udpread->data.buffer,
					 udpread->data.size,
					 0,
					 (sockaddr*) udpread->endpoint.GetSockAddr(),
					 (socklen_t*)&salen);
	
	if (nread < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			IOProcessor::Add(udpread); // try again
		else
			Log_Errno();
	}
	else
	{
		udpread->data.length = nread;
		Call(udpread->onComplete);
	}
					
	if (ev->flags & EV_EOF)
		Call(udpread->onClose);
}

void ProcessUDPWrite(struct kevent* ev)
{
	int			nwrite;
	UDPWrite*	udpwrite;

	udpwrite = (UDPWrite*) ev->udata;

	if (ev->data >= (int) udpwrite->data.length)
	{
		nwrite = sendto(udpwrite->fd,
						udpwrite->data.buffer,
						udpwrite->data.length,
						0,
						(const sockaddr*) udpwrite->endpoint.GetSockAddr(),
						ENDPOINT_SOCKADDR_SIZE);
		
		if (nwrite < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				IOProcessor::Add(udpwrite); // try again
			else
				Log_Errno();
		}
		else
		{
			if (nwrite == (int) udpwrite->data.length)
			{
				Call(udpwrite->onComplete);
			}
			else
			{
				Log_Trace("sendto() datagram fragmentation");
				IOProcessor::Add(udpwrite); // try again
			}
		}
	}
	
	if (ev->flags & EV_EOF)
		Call(udpwrite->onClose);
}

#endif // PLATFORM_DARWIN
