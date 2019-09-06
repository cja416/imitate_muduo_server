#pragma once
#include "noncopyable.h"
#include "MutexLock.h"
#include <pthread.h>
#include <errno.h>
#include <cstdint>
#include <time.h>

class Condition : noncopyable
{
public:
	explicit Condition(MutexLock &_mutex) :
		mutex(_mutex)
	{
		pthread_cond_init(&cond, NULL);		//��ʼ����������
	}
	~Condition()
	{
		pthread_cond_destroy(&cond);		//������������
	}
	void wait()
	{
		pthread_cond_wait(&cond, mutex.get());		//wait����3���£��������ȴ��������� mutex.get()����MutexLock���еĳ�Աmutex
	}
	void notify()
	{
		pthread_cond_signal(&cond);		//��������1��
	}
	void notifyAll()
	{
		pthread_cond_broadcast(&cond);		//��������
	}
	bool waitForSeconds(int seconds)
	{
		struct timespec abstime;
		clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec += static_cast<time_t>(seconds);
		return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);		//��ʱ�ȴ���ʽ����ڸ���ʱ��ǰ����û�����㣬�򷵻�ETIMEDOUT�������ȴ�
	}
private:
	MutexLock &mutex;
	pthread_cond_t cond;
};

