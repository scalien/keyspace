#include "System/Platform.h"
#ifdef PLATFORM_DARWIN

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#include "IOProcessor.h"
#include "System/Containers/List.h"
#include "System/Common.h"
#include "System/Log.h"
#include "System/Time.h"

// see http://wiki.netbsd.se/index.php/kqueue_tutorial

class FileOp;

static int				kq;			// the kqueue
static List<FileOp*>	fileops;	// the list of aio ops
static int				asyncOpPipe[2];

static bool AddKq(int ident, short filter, IOOperation* ioop);
static bool AddAio(FileOp* ioop);

static void ProcessAsyncOp();
static void ProcessTCPRead(struct kevent* ev);
static void ProcessTCPWrite(struct kevent* ev);
static void ProcessUDPRead(struct kevent* ev);
static void ProcessUDPWrite(struct kevent* ev);
static void ProcessFileOp(struct kevent* ev);

bool IOProcessor::Init(int maxfd_)
{
	rlimit rl;
	
	kq = kqueue();
	if (kq < 0)
	{
		Log_Errno();
		return false;
	}
	
	rl.rlim_cur = maxfd_;
	rl.rlim_max = maxfd_;
	if (setrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		Log_Errno();
	}
	
	// setup AIO
	if (!AddKq(SIGIO, EVFILT_SIGNAL, NULL))
		return false;
	
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
	close(asyncOpPipe[0]);
	close(asyncOpPipe[1]);
}

bool IOProcessor::Add(IOOperation* ioop)
{
	short	filter;
	
	if (ioop->type == FILE_READ || ioop->type == FILE_WRITE)
	{
		return AddAio((FileOp*) ioop);
	}
	else
	{
		if (ioop->type == TCP_READ || ioop->type == UDP_READ)
			filter = EVFILT_READ;
		else
			filter = EVFILT_WRITE;
		
		return AddKq(ioop->fd, filter, ioop);
	}
}

bool AddAio(FileOp* fileop)
{
#ifdef IOPROCESSOR_AIO
	memset(&(fileop->cb), 0, sizeof(struct aiocb));
	
	fileop->cb.aio_fildes	= fileop->fd;
	fileop->cb.aio_offset	= fileop->offset;
	fileop->cb.aio_buf		= fileop->data.buffer;
		
	fileop->cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	fileop->cb.aio_sigevent.sigev_signo = SIGIO;
	
	if (fileop->type == FILE_READ)
	{
		fileop->cb.aio_nbytes = fileop->nbytes;
		
		if (aio_read(&(fileop->cb)) < 0)
		{
			Log_Errno();
			return false;
		}
	} else
	{
		fileop->cb.aio_nbytes = fileop->data.length;
		
		if (aio_write(&(fileop->cb)) < 0)
		{
			Log_Errno();
			return false;
		}
	}

	fileop->active = true;	
	fileops.Add(fileop);

#endif // IOPROCESSOR_AIO

	return true;
}

bool AddKq(int ident, short filter, IOOperation* ioop)
{
	int				nev;
	struct kevent	ev;
	struct timespec timeout = { 0, 0 };
	
	if (kq < 0)
	{
		Log_Message("kq < 0");
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
	
	if (kq < 0)
	{
		Log_Message("kq < 0");
		return false;
	}

	// TODO Remove(FileOp*)
	
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
		return false;
	}
	
	ioop->active = false;
	
	return true;
}

bool IOProcessor::Poll(int sleep)
{
#define	MAX_KEVENTS 1
	
	uint64_t					called;
	int						i, nevents, wait;
	static struct kevent	events[MAX_KEVENTS];
	struct timespec			timeout;
	IOOperation*			ioop;
	
	called = Now();
	
	do
	{
		wait = sleep - (Now() - called);
		if (wait < 0) wait = 0;
		
		timeout.tv_sec = (time_t) floor(wait / 1000.0);
		timeout.tv_nsec = (wait - 1000 * timeout.tv_sec) * 1000000;
		
		nevents = kevent(kq, NULL, 0, events, SIZE(events), &timeout);
		
		if (nevents < 0)
		{
			Log_Errno();
			return false;
		}
		
		for (i = 0; i < nevents; i++)
		{
			if (events[i].flags & EV_ERROR)
				Log_Message(strerror(events[i].data));
			
			if (events[i].filter == EVFILT_SIGNAL)
			{
				// re-register for signal notification
				if (!AddKq(SIGIO, EVFILT_SIGNAL, NULL))
					return false;
				
				ProcessFileOp(&events[i]);
			}
			else
			{
				if (events[i].udata == NULL)
				{
					ProcessAsyncOp();

					// re-register for notification
					if (!AddKq(asyncOpPipe[0], EVFILT_READ, NULL))
						return false;
					
					continue;
				}
				
				ioop = (IOOperation*) events[i].udata;
				
				ioop->active = false;
				
				if (ioop && ioop->type == TCP_READ && (events[i].filter & EVFILT_READ))
					ProcessTCPRead(&events[i]);
				else if (ioop && ioop->type == TCP_WRITE && (events[i].filter & EVFILT_WRITE))
					ProcessTCPWrite(&events[i]);
				else if (ioop && ioop->type == UDP_READ && (events[i].filter & EVFILT_READ))
					ProcessUDPRead(&events[i]);
				else if (ioop && ioop->type == UDP_WRITE && (events[i].filter & EVFILT_WRITE))
					ProcessUDPWrite(&events[i]);
			}
		}
	} while (nevents > 0 && wait > 0);
	
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
	} else
	{
		if (tcpread->requested == IO_READ_ANY)
			readlen = tcpread->data.size - tcpread->data.length;
		else
			readlen = tcpread->requested - tcpread->data.length;
		readlen = min(ev->data, readlen);
		if (readlen > 0)
		{
			nread = read(tcpread->fd, tcpread->data.buffer + tcpread->data.length, readlen);
			if (nread < 0)
			{
				if (errno == EWOULDBLOCK || errno == EAGAIN)
					IOProcessor::Add(tcpread);
				else if (!(ev->flags & EV_EOF))
					Log_Errno();
			} else
			{
				tcpread->data.length += nread;
				if (tcpread->requested == IO_READ_ANY || (int) tcpread->data.length == tcpread->requested)
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
	writelen = min(ev->data, writelen);
	
	if (writelen > 0)
	{
		nwrite = write(tcpwrite->fd, tcpwrite->data.buffer, writelen);
		if (nwrite < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				IOProcessor::Add(tcpwrite);
			else if (!(ev->flags & EV_EOF))
				Log_Errno();
		} else
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
	
	salen = sizeof(udpread->endpoint.sa);
	nread = recvfrom(udpread->fd, udpread->data.buffer, udpread->data.size, 0,
				(sockaddr*)&udpread->endpoint.sa, (socklen_t*)&salen);
	if (nread < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			IOProcessor::Add(udpread); // try again
		else
			Log_Errno();
	} else
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
		nwrite = sendto(udpwrite->fd, udpwrite->data.buffer, udpwrite->data.length, 0,
					(const sockaddr*)&udpwrite->endpoint.sa, sizeof(udpwrite->endpoint.sa));
		if (nwrite < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
				IOProcessor::Add(udpwrite); // try again
			else
				Log_Errno();
		} else
		{
			if (nwrite == (int) udpwrite->data.length)
			{
				Call(udpwrite->onComplete);
			} else
			{
				Log_Message("sendto() datagram fragmentation");
				IOProcessor::Add(udpwrite); // try again
			}
		}
	}
	
	if (ev->flags & EV_EOF)
		Call(udpwrite->onClose);
}

void ProcessFileOp(struct kevent*)
{
#ifdef IOPROCESSOR_AIO
	int			ret, nbytes;
	FileOp**	it;
	FileOp*		fileop;

	for (it = fileops.Head(); it != NULL; /* advanced in body */)
	{
		fileop = *it;
		
		ret = aio_error(&fileop->cb);
		if (ret == EINPROGRESS)
		{
			it = fileops.Next(it);
			continue;
		}

		nbytes = aio_return(&fileop->cb);
		if (nbytes >= 0)
		{
			if (fileop->type == FILE_READ)
				fileop->data.length = nbytes;
			else
				fileop->nbytes = nbytes;
		}
	
		fileops.Remove(it);
		fileop->active = false;

		if (ret == 0)
			Call(fileop->onComplete);
		else
			Call(fileop->onClose); // todo: should be onError
		
		it = fileops.Head();		
	}
#endif // IOPROCESSOR_AIO
}

#endif // PLATFORM_DARWIN
