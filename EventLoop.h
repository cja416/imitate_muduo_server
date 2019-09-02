#pragma once
#include "Thread.h"
#include "Epoll.h"
#include "Logging.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "Util.h"
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
using namespace std;

class EventLoop
{
public:
	typedef std::function<void()> Functor;
	EventLoop();
	~EventLoop();
	void loop();
	void quit();
	void runInLoop(Functor&& cb);
	void queueInLoop(Functor&& cb);			//�������
	bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }		//�жϵ�ǰ�߳�id�Ƿ���ڱ�eventloop�󶨵��߳�id
	void assertInLoopThread()
	{
		assert(isInLoopThread());
	}
	void shutdown(shared_ptr<Channel> channel)
	{
		shutDownWR(channel->getFd());
	}
	void removeFromPoller(shared_ptr<Channel> channel)
	{
		//shutDownWR(channel->getFd());
		poller_->epoll_del(channel);
	}
	void updatePoller(shared_ptr<Channel> channel, int timeout = 0)
	{
		poller_->epoll_mod(channel, timeout);
	}
	void addToPoller(shared_ptr<Channel> channel, int timeout = 0)
	{
		poller_->epoll_add(channel, timeout);
	}

private:
	// ����˳�� wakeupFd_ > pwakeupChannel_
	bool looping_;
	shared_ptr<Epoll> poller_;
	int wakeupFd_;
	bool quit_;
	bool eventHandling_;
	mutable MutexLock mutex_;			//ÿ��eventloop�и�mutex		mutableͻ�Ƴ�Ա���������const����
	std::vector<Functor> pendingFunctors_;
	bool callingPendingFunctors_;
	const pid_t threadId_;
	shared_ptr<Channel> pwakeupChannel_;

	void wakeup();
	void handleRead();
	void doPendingFunctors();
	void handleConn();
};