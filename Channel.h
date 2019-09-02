#pragma once
#include "Timer.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
//#include <sys/epoll.h>


class EventLoop;
class HttpData;


class Channel
{
private:
	typedef std::function<void()> CallBack;
	EventLoop *loop_;
	int fd_;
	__uint32_t events_;
	__uint32_t revents_;
	__uint32_t lastEvents_;
	/*
	struct epoll_event {				epoll的事件结构体中数据类型为这个
	__uint32_t events;      // Epoll events 
	epoll_data_t data;      // User data variable 
	};
	*/


	// 方便找到上层持有该Channel的对象			是一根棉线
	std::weak_ptr<HttpData> holder_;

private:
	int parse_URI();
	int parse_Headers();
	int analysisRequest();

	CallBack readHandler_;
	CallBack writeHandler_;
	CallBack errorHandler_;
	CallBack connHandler_;

public:
	Channel(EventLoop *loop);
	Channel(EventLoop *loop, int fd);
	~Channel();
	int getFd();
	void setFd(int fd);

	void setHolder(std::shared_ptr<HttpData> holder)
	{
		holder_ = holder;
	}
	std::shared_ptr<HttpData> getHolder()
	{
		std::shared_ptr<HttpData> ret(holder_.lock());
		return ret;
	}

	void setReadHandler(CallBack &&readHandler)
	{
		readHandler_ = readHandler;
	}
	void setWriteHandler(CallBack &&writeHandler)
	{
		writeHandler_ = writeHandler;
	}
	void setErrorHandler(CallBack &&errorHandler)
	{
		errorHandler_ = errorHandler;
	}
	void setConnHandler(CallBack &&connHandler)
	{
		connHandler_ = connHandler;
	}

	void handleEvents()
	{
		events_ = 0;
		if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
		{
			events_ = 0;
			return;
		}
		if (revents_ & EPOLLERR)
		{
			if (errorHandler_) errorHandler_();
			events_ = 0;
			return;
		}
		if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))		//revent在事件分发时被决定
		{
			handleRead();			//最终会调用我们set的回调函数，即handThisConn等
		}
		if (revents_ & EPOLLOUT)
		{
			handleWrite();
		}
		handleConn();					//不是处理输入输出的话就是处理连接的情况，包括清零定时器、设为发完剩余数据、或者关闭连接
	}
			//在使用 epoll 时，对端正常断开连接（调用 close()），在服务器端会触发一个 epoll 事件。在低于 2.6.17 版本的内核中，
			//这个 epoll 事件一般是 EPOLLIN，即 0x1，代表连接可读，但recv到的是0；
			//在使用 2.6.17 之后版本内核的服务器系统中，对端连接断开触发的 epoll 事件会包含 EPOLLIN | EPOLLRDHUP，即 0x2001
	void handleRead();
	void handleWrite();
	void handleError(int fd, int err_num, std::string short_msg);
	void handleConn();

	void setRevents(__uint32_t ev)
	{
		revents_ = ev;
	}

	void setEvents(__uint32_t ev)
	{
		events_ = ev;
	}
	__uint32_t& getEvents()
	{
		return events_;
	}

	bool EqualAndUpdateLastEvents()
	{
		bool ret = (lastEvents_ == events_);
		lastEvents_ = events_;
		return ret;
	}

	__uint32_t getLastEvents()
	{
		return lastEvents_;
	}

};

typedef std::shared_ptr<Channel> SP_Channel;