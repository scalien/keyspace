#include "System/Events/EventLoop.h"
#include "System/IO/IOProcessor.h"
#include "HubSim.h"

IOProcessor* ioproc;

int main(int argc, char* argv[])
{
	HubSim hubSim;
	EventLoop* eventloop;
	
	Log_SetTimestamping(true);
	
	
	IOProcessor::Init();
	
	hubSim.CreateNode("127.0.0.1:4001", 5000);
	hubSim.CreateNode("127.0.0.1:4000", 5001);
	
	EventLoop::Run();
	return 0;
}	
