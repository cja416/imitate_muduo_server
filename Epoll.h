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
	std::shared_ptr<Channel> fd2chan_[MAXFDS];//��Ȼ����fd��Ϊ��ȡ������Ϊ�β���map��   ÿ��channel��Ӧһ��fd������epoll�ҽڵ���ʱ��ȷ����һһ��Ӧ��ϵ
	std::shared_ptr<HttpData> fd2http_[MAXFDS];		//ͨ��channel  -  fd   - HttpData   ������ ������-�ļ�������-http����ҵ���ӳ���ϵ
	TimerManager timerManager_;
};