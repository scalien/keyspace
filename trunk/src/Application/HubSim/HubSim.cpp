#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "System/IO/IOOperation.h"
#include "System/IO/Socket.h"
#include "HubSim.h"
#include "Link.h"

#define MAX_FD		64
#define MAX_PORT	10

extern IOProcessor* ioproc;


class UDPPort;
class UDPReader
{
public:
	virtual void OnRead(UDPPort* udpPort, Endpoint &ep, const char* data, int count) = 0;
};

class Node;

class UDPPort : public Port
{
public:
	UDPRead		udpread;
	UDPWrite	udpwrite;
	Socket		socket;
	int			port;
	UDPPort*	peer;
	Node*		node;
	UDPReader*	reader;
	MFunc<UDPPort> onWrite;
	MFunc<UDPPort> onClose;
	MFunc<UDPPort> onRead;
	ByteArray<1024> readBuffer;
	
	UDPPort() :
	onRead(this, &UDPPort::OnRead),
	onWrite(this, &UDPPort::OnWrite),
	onClose(this, &UDPPort::OnClose)
	{
		Log_Trace();
		port = 0;
		peer = NULL;
		node = NULL;
		reader = NULL;
	}
	
	void Init(Node* node_, int port_)
	{
		port = port_;
		node = node_;
		
		socket.Create(UDP);
		socket.Bind(port);
		socket.SetNonblocking();
	}
	
	void OnWrite()
	{
		
	}
	
	void OnClose()
	{
		
	}
	
	void OnRead()
	{
		reader->OnRead(this, udpread.endpoint, readBuffer.data, readBuffer.length);
	}
	
	virtual void Read(UDPReader* reader_)
	{
		reader = reader_;
		
		udpread.fd = socket.fd;
		udpread.data = readBuffer;
		udpread.onClose = &onClose;
		udpread.onComplete = &onRead;
		
		ioproc->Add(&udpread);
	}
	
	virtual void Write(Endpoint &dst, const char* data, int count)
	{
		udpwrite.fd = socket.fd;
		udpwrite.data.buffer = (char*) data;
		udpwrite.data.size = count;
		udpwrite.data.length = count;
		udpwrite.onClose = &onClose;
		udpwrite.onComplete = &onWrite;
		udpwrite.endpoint = dst;
		
		ioproc->Add(&udpwrite);
	}
};

class Node : public UDPReader
{
public:
	HubSim		&hubSim;
	Endpoint	endpoint;
	UDPPort		udpPort;
	
	Node(HubSim &hubSim_) :
	hubSim(hubSim_)
	{
		Log_Trace();
	}
	
	void Init(char* ip, int port)
	{
		endpoint.Set(ip);
		udpPort.Init(this, port);
		udpPort.Read(this);
	}
	
		
	void OnRead(UDPPort* , Endpoint &src, const char* data, int count)
	{
		Node* peerNode;
		Link* link;
	
		peerNode = hubSim.GetNode(src);
		link = hubSim.GetLink(this, peerNode);
		link->WriteUDP(&peerNode->udpPort, endpoint, data, count);
		udpPort.Read(this);
	}	
};


HubSim::HubSim()
{
	numNodes = 0;
	numLinks = 0;
	
	goodLink = new Link;
	packetLossLink = new PacketLossLink;
	packetLossLink->SetLossProbability(0.9);
	reorderingLink = new ReorderingLink;
	reorderingLink->SetReorderProbability(0.5);
	AddLink(goodLink);
}

void HubSim::AddLink(Link* link)
{
	links[numLinks++] = link;
}

void HubSim::CreateNode(char *ip, int port)
{
	Node* node = new Node(*this);
	nodes[numNodes] = node;
	numNodes++;
	
	node->Init(ip, port);
}

Node* HubSim::GetNode(Endpoint &ep)
{
	for (int i = 0; i < numNodes; i++)
	{
		if (nodes[i]->endpoint == ep)
			return nodes[i];
	}
	return NULL;
}

Link* HubSim::GetLink(Node* , Node* )
{
//	return goodLink;
//	return packetLossLink;
	return reorderingLink;
}
