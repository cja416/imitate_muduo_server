#include "EventLoop.h"
#include "Logging.h"
#include "Util.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
using namespace std;

__thread EventLoop* t_loopInThisThread = 0;  
 //EventLoop* t_loopInThisThread = 0;				正确的是用__thread修饰  表示此io线程拥有的eventloop（一一对应）

int creatEventfd() {
	int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		LOG << "Failed in eventfd";
		abort();
	}
	return evtfd;
}


EventLoop::EventLoop()			//构造函数
:	looping_(false),
	poller_(new Epoll()),					//这里会新建epoll的socket描述符
	wakeupFd_(creatEventfd()),				//这里新建事件唤醒的文件描述符
	quit_(false),
	eventHandling_(false),
	callingPendingFunctors_(false),
	threadId_(CurrentThread::tid()),
	pwakeupChannel_(new Channel(this,wakeupFd_))		//跟channel绑定的是唤醒描述符
{
	if (t_loopInThisThread) {
		//LOG << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
	}
	else {
		t_loopInThisThread = this;
	}

	pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);		//配置唤醒的channel事件
	pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead,this));
	pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
	poller_->epoll_add(pwakeupChannel_, 0);					//用来唤醒的eventfd挂到红黑树上
}
	
void EventLoop::handleConn() {
	//poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET | EPOLLONESHOT), 0);原始写法
	updatePoller(pwakeupChannel_, 0);
}

EventLoop::~EventLoop() {
	//wakeupChannel_->disableAll();
	//wakeupChannel_->remove();
	close(wakeupFd_);
	t_loopInThisThread = nullptr;
}

void EventLoop::wakeup() {				//到底啥时候唤醒的
	uint64_t one = 1;
	ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);//写1来唤醒线程
	if (n != sizeof one) {
		LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
	}
}

void EventLoop::handleRead() {			//这是IO线程的pwakeupChannel_绑定的处理函数
	uint64_t one = 1;
	ssize_t n = readn(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
	}
	//pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);		
}


void EventLoop::runInLoop(Functor&& cb) {
	if (isInLoopThread())					//如何判断是否在IO线程中？
		cb();
	else
		queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb) {
	{
		MutexLockGuard lock(mutex_);				//主线程与IO线程争用IO线程的eventloop的锁,保证放进任务队列的时候IO线程不会正在处理任务
		pendingFunctors_.emplace_back(std::move(cb));
	}

	if (!isInLoopThread() || callingPendingFunctors_)			//在向IO线程分发未决事件时，还是跑一下程序看看
		wakeup();
}

void EventLoop::loop() {
	assert(!looping_);		//确保线程中的eventloop未启动
	assert(isInLoopThread());	//确保eventloop已绑定线程
	looping_ = true;
	quit_ = false;
	//LOG << "EventLoop " << this << " start looping";
	std::vector<SP_Channel> ret;		//SP即shared_ptr
	while (!quit_) {					//不断循环取出eventloop中epoll获得的事件
		//cout << "doing" << endl;
		ret.clear();
		ret = poller_->poll();		//获得带有事件信息的channel指针数组
		eventHandling_ = true;
		for (auto &it : ret)
			it->handleEvents();		//线程逐个处理,在poll中把epoll_wait拿到的事件都放在ret数组中channel对象的revent里，这里逐个拿出来处理，
		eventHandling_ = false;								//每个channel绑定了回调处理函数如，accpetchannel绑定了 Server::handNewConn()
		doPendingFunctors();								//而每个IO线程的channel则是绑定了HttpData::handleRead()或者HttpData::handleWrite
		poller_->handleExpired();//这个函数是Epoll类中的函数，每个IO循环中处理超时连接
	}
	looping_ = false;
}

void EventLoop::doPendingFunctors() {
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;
	{
		MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);		//pendingFunctors_   未决事件,多为将读写fd挂到io线程的epoll上
	}
	
	for (size_t i = 0; i < functors.size(); ++i)
		functors[i]();							//逐一调用未决函数      他的产生是调用了runInLoop时，eventloop未占有一个线程
																		//(还是说此线程被阻塞监听在epoll上，应该将他唤醒)  IO线程阻塞监听时，收到的分发事件暂存在这里
	callingPendingFunctors_ = false;
}

void EventLoop::quit() {
	quit_ = true;
	if (!isInLoopThread()) {
		wakeup();
	}
}
