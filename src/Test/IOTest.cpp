#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

#include "System/Events/EventLoop.h"

IOProcessor* ioproc;
ByteArray<32> ba;
FileWrite fw;
FileRead fr;
char c = 'a';

void OnFileWrite()
{
	if (c++ < 'z')
	{
		for (int i = 0; i < ba.size - 1; i++)
			ba.buffer[i] = c;
		ba.buffer[ba.size - 1] = '\n';
		
		fw.offset += fw.nbytes;
		ioproc->Add(&fw);
	}
	else
	{
		exit(0);
	}
}

void OnFileRead()
{
	if (fr.data.length < 1)
		exit(0);
		
	printf("%d, %.32s", fr.data.length, fr.data.buffer);
	
	fr.nbytes = fr.data.size;
	fr.offset += fr.data.length;
	ioproc->Add(&fr);
}

int main(int argc, char* argv[])
{
	EventLoop*	eventloop;
	char*		filename = "test.txt";
#ifdef PLATFORM_DARWIN
	const int	DIRECT_MODE = 0;
#else
	const int	DIRECT_MODE = 0;
#endif
	
	if (argc > 2)
	{
		filename = argv[1];
	}
	
	ioproc = IOProcessor::Get();
	eventloop = new EventLoop(ioproc);
	
	ioproc->Init();
		
	for (int i = 0; i < ba.size - 1; i++)
		ba.buffer[i] = c;
	ba.buffer[ba.size - 1] = '\n';
	ba.length = ba.size;
	
	if ((fw.fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC | DIRECT_MODE, 0600)) < 0)
	{
		Log_Errno();
		return 1;
	}
	
	fw.data = ba;
	fw.nbytes = ba.length;
	CFunc onFileWrite(&OnFileWrite);
	fw.onComplete = &onFileWrite;
	ioproc->Add(&fw);
	/*
	
	if ((fr.fd = open(filename, O_RDONLY)) < 0)
	{
		Log_Errno();
		return 1;
	}
	
	fr.data = ba;
	fr.nbytes = ba.size;
	CFunc onFileRead(&OnFileRead);
	fr.onComplete = &onFileRead;
	ioproc->Add(&fr);
	*/
	
	eventloop->Run();	
}
