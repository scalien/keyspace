#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include "System/Containers/List.h"

class Callable;

class ThreadPool
{
public:
	ThreadPool(int numThread);
	~ThreadPool();

	void			Start();
	void			Stop();

	void			Execute(Callable *callable);
	
private:
	pthread_t*		threads;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
	List<Callable*>	callables;
	int				numCallable;
	int				numActive;
	int				numThread;
	bool			running;
	
	static void*	thread_function(void *param);
	void			ThreadFunction();
};

#endif
