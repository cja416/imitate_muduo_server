#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
	:baseLoop_(baseLoop),
	started_(false),
	numThreads_(numThreads),
	next_(0)
{
	if (numThreads_ <= 0)
	{
		LOG << "numThreads_ <= 0";
		abort();
	}
}

void EventLoopThreadPool::start() {			//在Server::start()中调用开始
	baseLoop_->assertInLoopThread();		//判断是否在循环线程中
	started_ = true;
	for (int i = 0; i < numThreads_; ++i) {
		std::shared_ptr<EventLoopThread> t(new EventLoopThread());	//根据传入的线程数创建指向loop线程的智能指针
		threads_.push_back(t);		//将管理循环线程的指针填入已创建线程的数组，便于统一管理
		loops_.push_back(t->startLoop());	//开启新建线程的循环，并放入管理循环的数组中
	}
}

EventLoop* EventLoopThreadPool::getNextLoop() {
	baseLoop_->assertInLoopThread();
	assert(started_);
	EventLoop* loop = baseLoop_;	//临时变量赋值为主线程
	if (!loops_.empty())		//判断他管理的循环线程数组中有没有 正在循环的线程
	{
		loop = loops_[next_];
		next_ = (next_ + 1) % numThreads_;			//这就是所谓round robin算法
	}
	return loop;		//取到了一个IO线程loop
}