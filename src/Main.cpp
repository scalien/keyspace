#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "Framework/Transport/TransportTCPReader.h"

TransportTCPReader reader;

void OnRead()
{
	ByteString bs;
	reader.GetMessage(bs);
	Log_Message("Received message: %.*s", bs.length, bs.buffer);
}

int main(int argc, char* argv[])
{
	IOProcessor*	ioproc;
	EventLoop*		eventloop;
	
	Log_SetTimestamping(true);
	
	ioproc = IOProcessor::New();
	eventloop = new EventLoop(ioproc);
	
	ioproc->Init();

	reader.Init(ioproc, 8080);
	CFunc onRead(&OnRead);
	reader.SetOnRead(&onRead);
	
	eventloop->Run();
}
