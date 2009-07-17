#ifndef CHANNEL_H
#define CHANNEL_H

#include <pthread.h>

class Channel
{
public:
	Channel();
	
	int					Send(const char* buf, int count);
	int					Receive(char* buf, int count);
private:
	bool				sending;
	pthread_mutex_t		send_mutex;
	pthread_cond_t		send_cond;
	
	bool				receiving;
	pthread_mutex_t		recv_mutex;
	pthread_cond_t		recv_cond;
	
	const char*			buf;
	int					count;
};

#endif
