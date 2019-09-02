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
	struct epoll_event {				epoll���¼��ṹ������������Ϊ���
	__uint32_t events;      // Epoll events 
	epoll_data_t data;      // User data variable 
	};
	*/


	// �����ҵ��ϲ���и�Channel�Ķ���			��һ������
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
		if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))		//revent���¼��ַ�ʱ������
		{
			handleRead();			//���ջ��������set�Ļص���������handThisConn��
		}
		if (revents_ & EPOLLOUT)
		{
			handleWrite();
		}
		handleConn();					//���Ǵ�����������Ļ����Ǵ������ӵ�������������㶨ʱ������Ϊ����ʣ�����ݡ����߹ر�����
	}
			//��ʹ�� epoll ʱ���Զ������Ͽ����ӣ����� close()�����ڷ������˻ᴥ��һ�� epoll �¼����ڵ��� 2.6.17 �汾���ں��У�
			//��� epoll �¼�һ���� EPOLLIN���� 0x1���������ӿɶ�����recv������0��
			//��ʹ�� 2.6.17 ֮��汾�ں˵ķ�����ϵͳ�У��Զ����ӶϿ������� epoll �¼������ EPOLLIN | EPOLLRDHUP���� 0x2001
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