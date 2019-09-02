#include "EventLoopThread.h"
#include <functional>

EventLoopThread::EventLoopThread()
	:loop_(NULL),
	exiting_(false),
	thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),  //绑定线程的函数
	mutex_(),
	cond_(mutex_)
{}

EventLoopThread::~EventLoopThread() {		//析构，退出循环且join线程
	exiting_ = true;
	if (loop_ != NULL) {
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop() {		//由EventLoopThreadPool::start()调用
	assert(!thread_.started());
	thread_.start();

	{
		MutexLockGuard lock(mutex_);		//将锁搞成一个类，在函数段内创建临时锁对象并加锁，当退出函数段时自动调用对象析构函数进行解锁
				// 一直等到threadFun在Thread里真正跑起来，创建好的线程会运行threadFunc函数，创建loop并唤醒主线程，
		while (loop_ == NULL)
			cond_.wait();		//等在条件变量上 ,	条件变量也封装为一个条件类，并且this类中包含了cond的对象,一旦真正的线程创建完毕了就被唤醒
	}
	return loop_;
}


void EventLoopThread::threadFunc() {
	EventLoop loop;
	{
		MutexLockGuard lock(mutex_);
		loop_ = &loop;		//引用	,到这里loop_就不为NULL了，即条件变量等到了
		cond_.notify();
	}

	loop.loop();
	//assert(exiting_);
	loop_ = NULL;			//就是起到一个EventLoopThread创建EventLoop并启动事件循环的作用
}
