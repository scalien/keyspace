#include "System/Common.h"
#include "Channel.h"

Channel::Channel()
{
	sending = true;
	receiving = false;
	
	pthread_mutex_init(&send_mutex, NULL);
	pthread_cond_init(&send_cond, NULL);
	
	pthread_mutex_init(&recv_mutex, NULL);
	pthread_cond_init(&recv_cond, NULL);
}

int Channel::Send(const char* buf_, int count_)
{
	pthread_mutex_lock(&send_mutex);
	while (!sending)
		pthread_cond_wait(&send_cond, &send_mutex);
		
	buf = buf_;
	count = count_;
	
	sending = false;
	pthread_mutex_lock(&send_mutex);
	
	pthread_mutex_lock(&recv_mutex);
	receiving = true;
	pthread_cond_signal(&recv_cond);
	pthread_mutex_lock(&recv_mutex);
	
	return count;
}

int Channel::Receive(char* buf_, int count_)
{
	pthread_mutex_lock(&recv_mutex);
	while (!receiving)
		pthread_cond_wait(&recv_cond, &recv_mutex);

	count_ = min(count, count_);
	memcpy(buf_, this->buf, count_);
	receiving = false;
	
	pthread_mutex_unlock(&recv_mutex);
	
	pthread_mutex_lock(&send_mutex);
	sending = true;
	pthread_cond_signal(&send_cond);
	pthread_mutex_unlock(&send_mutex);
	
	return count_;
}
