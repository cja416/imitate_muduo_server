#include "Channel.h"
#include "Util.h"
#include "Epoll.h"
#include "EventLoop.h"
#include <unistd.h>
#include <queue>
#include <cstdlib>
#include <iostream>
using namespace std;

Channel::Channel(EventLoop* loop)
	:loop_(loop),
	 events_(0),
	 lastEvents_(0)
{}

Channel::Channel(EventLoop* loop, int fd)		//与eventloop、唤醒描述符绑定
	: loop_(loop),
	fd_(fd),
	events_(0),
	lastEvents_(0)
{}

Channel::~Channel()
{
	//loop_->poller_->epoll_del(fd, events_);
	//close(fd_);
}

int Channel::getFd()
{
	return fd_;
}

void Channel::setFd(int fd)
{
	fd_ = fd;
}

void Channel::handleRead() {
	if (readHandler_) 
	{
		readHandler_();			//调用我们传进去的回调函数
	}
}

void Channel::handleWrite()
{
	if (writeHandler_)
	{
		writeHandler_();
	}
}

void Channel::handleConn()
{
	if (connHandler_)
	{
		connHandler_();
	}
}