#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "System/Events/Callable.h"
#include "System/IO/Socket.h"
#include "System/IO/IOOperation.h"
#include "System/IO/IOProcessor.h"
#include "System/Containers/Queue.h"
#include "TCPConn.h"

class TCPServer
{
public:
	TCPServer();
	virtual ~TCPServer() {}
	
	bool				Init(int port);
	void				Close();
	
protected:
	TCPRead				tcpread;
	Socket				listener;
	MFunc<TCPServer>	onConnect;
	
	virtual void		OnConnect() = 0;
};


template<class T, class Conn, int bufSize = MAX_TCP_MESSAGE_SIZE>
class TCPServerT
{
public:
	TCPServerT() :
	onConnect(this, &TCPServerT<T, Conn, bufSize>::OnConnect)
	{
		numActive = 0;
	}

	~TCPServerT()
	{
		Conn* conn;
		while ((conn = dynamic_cast<Conn*>(conns.Get())) != NULL)
			delete conn;
	}
	
	bool Init(int port_, int backlog_)
	{
		bool ret;
		
		Log_Trace();
		backlog = backlog_;
	
		ret = listener.Create(Socket::TCP);
		if (!ret)
			return false;
		ret = listener.Listen(port_);
		if (!ret)
			return false;	
		ret = listener.SetNonblocking();
		if (!ret)
			return false;

		tcpread.fd = listener.fd;
		tcpread.listening = true;
		tcpread.onComplete = &onConnect;
		
		return IOProcessor::Add(&tcpread);	
	}
	
	void Close()
	{
//		IOProcessor::Remove(&tcpread);
		listener.Close();
	}

	void DeleteConn(Conn* conn)
	{
		Log_Trace();
		
		numActive--;
		assert(numActive >= 0);
		
		if (conns.Size() >= backlog)
			delete conn;
		else
			conns.Append((TCPConn<bufSize>*)conn);
	}

protected:
	typedef Queue<TCPConn<bufSize>, &TCPConn<bufSize>::next> ConnList;
	
	TCPRead				tcpread;
	Socket				listener;
	MFunc<TCPServerT>	onConnect;
	int					backlog;
	ConnList			conns;
	int					numActive;
	
	void OnConnect()
	{
		T* pT = static_cast<T*>(this);
		Conn* conn = pT->GetConn();
		numActive++;
		if (listener.Accept(&(conn->GetSocket())))
		{
			conn->GetSocket().SetNonblocking();
			conn->GetSocket().SetNodelay();
			pT->InitConn(conn);
		}
		else
		{
			Log_Trace("Accept() failed");
			pT->DeleteConn(conn);
		}
		
		IOProcessor::Add(&tcpread);		
	}
	
	Conn* GetConn()
	{
		if (conns.Size() > 0)
			return dynamic_cast<Conn*>(conns.Get());
		
		return new Conn;
	}
	
	void InitConn(Conn* conn)
	{
		T* pT = static_cast<T*>(this);
		conn->Init(pT);
	}
	
};

#endif
