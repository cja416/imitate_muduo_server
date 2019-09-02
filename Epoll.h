#pragma once
#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>

class Epoll
{
public:
	Epoll();
	~Epoll();
	void epoll_add(SP_Channel request, int timeout);
	void epoll_mod(SP_Channel request, int timeout);
	void epoll_del(SP_Channel request);
	std::vector<std::shared_ptr<Channel>> poll();
	std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
	void add_timer(std::shared_ptr<Channel> request_data, int timeout);
	int getEpollFd()
	{
		return epollFd_;
	}
	void handleExpired();
private:
	static const int MAXFDS = 100000;
	int epollFd_;
	std::vector<epoll_event> events_;
	std::shared_ptr<Channel> fd2chan_[MAXFDS];//既然是以fd作为存取索引，为何不用map？   每个channel对应一个fd，在往epoll挂节点是时候确定了一一对应关系
	std::shared_ptr<HttpData> fd2http_[MAXFDS];		//通过channel  -  fd   - HttpData   建立了 运载器-文件描述符-http请求业务的映射关系
	TimerManager timerManager_;
};