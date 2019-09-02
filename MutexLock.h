#pragma once
#include "noncopyable.h"
#include <pthread.h>
#include <cstdio>

class MutexLock : noncopyable			//这个类是不传参数，可以使用他来创建锁和上锁（就是把线程锁的api封装）
{
public:
	MutexLock()
	{
		pthread_mutex_init(&mutex, NULL);
	}
	~MutexLock()
	{
		pthread_mutex_lock(&mutex);
		pthread_mutex_destroy(&mutex);
	}
	void lock()
	{
		pthread_mutex_lock(&mutex);
	}
	void unlock()
	{
		pthread_mutex_unlock(&mutex);
	}
	pthread_mutex_t *get()
	{
		return &mutex;
	}
private:
	pthread_mutex_t mutex;

	// 友元类不受访问权限影响
private:
	friend class Condition;
};


class MutexLockGuard : noncopyable		//这个类是传进去一个已创建的mutex，直接就上锁了
{
public:
	explicit MutexLockGuard(MutexLock &_mutex) :
		mutex(_mutex)
	{
		mutex.lock();
	}
	~MutexLockGuard()
	{
		mutex.unlock();
	}
private:
	MutexLock &mutex;
};