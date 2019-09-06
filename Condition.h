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
		pthread_cond_init(&cond, NULL);		//初始化条件变量
	}
	~Condition()
	{
		pthread_cond_destroy(&cond);		//销毁条件变量
	}
	void wait()
	{
		pthread_cond_wait(&cond, mutex.get());		//wait会做3件事：解锁、等待、加锁， mutex.get()返回MutexLock类中的成员mutex
	}
	void notify()
	{
		pthread_cond_signal(&cond);		//唤醒至少1个
	}
	void notifyAll()
	{
		pthread_cond_broadcast(&cond);		//唤醒所有
	}
	bool waitForSeconds(int seconds)
	{
		struct timespec abstime;
		clock_gettime(CLOCK_REALTIME, &abstime);
		abstime.tv_sec += static_cast<time_t>(seconds);
		return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);		//计时等待方式如果在给定时刻前条件没有满足，则返回ETIMEDOUT，结束等待
	}
private:
	MutexLock &mutex;
	pthread_cond_t cond;
};

