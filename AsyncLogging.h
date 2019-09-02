#pragma once
#include "CountDownLatch.h"
#include "MutexLock.h"
#include "Thread.h"
#include "LogStream.h"
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>

class AsyncLogging : noncopyable
{
public:
	AsyncLogging(const std::string basename, int flushInterval = 2);
	~AsyncLogging()
	{
		if (running_)
			stop();
	}
	void append(const char* logline, int len);

	void start()
	{
		running_ = true;
		thread_.start();
		latch_.wait();					//启动时先阻塞在此条件变量上（count>0）,然后等一个output函数来notify他
	}

	void stop()
	{
		running_ = false;
		cond_.notify();
		thread_.join();
	}


private:
	void threadFunc();
	typedef FixedBuffer<kLargeBuffer> Buffer;			//将FixedBuffer<4000000>  定义为Buffer
	typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
	typedef std::shared_ptr<Buffer> BufferPtr;
	const int flushInterval_;
	bool running_;
	std::string basename_;
	Thread thread_;
	MutexLock mutex_;
	Condition cond_;
	BufferPtr currentBuffer_;
	BufferPtr nextBuffer_;
	BufferVector buffers_;
	CountDownLatch latch_;
};