#include "test.h"
#include "System/Events/Callable.h"
#include "System/ThreadPool.h"
#include "System/Time.h"

class DatabaseOperation
{
public:
	DatabaseOperation() : num(0), request(this, &DatabaseOperation::Request) {}
	
	int							num;
	
	void						Request();
	MFunc<DatabaseOperation>	request;
};

void DatabaseOperation::Request()
{
	TEST_LOG("thread = %08x, num = %d", (unsigned long) pthread_self(), num);
	Sleep(1000);
}

int ThreadPoolTest()
{
	ThreadPool			tp(1);
	DatabaseOperation	ops[100];
	
	tp.Start();
	for (int i = 0; i < 100; i++)
	{
		DatabaseOperation *op = &ops[i];
		op->num = i;
		
		tp.Execute(&op->request);
	}
	
	Sleep(10000);
	
	return TEST_SUCCESS;
}

TEST_MAIN(ThreadPoolTest);
