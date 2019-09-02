#include "Epoll.h"
#include "Util.h"
#include "Logging.h"
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <queue>
#include <deque>
#include <assert.h>

#include <arpa/inet.h>
#include <iostream>
using namespace std;

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

typedef shared_ptr<Channel> SP_Channel;

Epoll::Epoll()
	:epollFd_(epoll_create1(EPOLL_CLOEXEC)),			//切换线程时会自动关闭本epoll_fd
	events_(EVENTSNUM)
{
	assert(epollFd_ > 0);
}

Epoll::~Epoll() 
{}

//注册新描述符
void Epoll::epoll_add(SP_Channel request, int timeout) {
	int fd = request->getFd();
	if (timeout > 0) {
		add_timer(request, timeout);		//定时器
		fd2http_[fd] = request->getHolder();		//fd2http_指向了存放若干HttpData对象的数组 
	}
	struct epoll_event event;
	event.data.fd = fd;				//配置欲注册文件描述符的有关信息
	event.events = request->getEvents();		//channel可以返回得到的events

	request->EqualAndUpdateLastEvents();		//更新最近发生的事件（将本事件设为最近发生的事件)

	fd2chan_[fd] = request;		//既然是以fd作为存取索引，为何不用map？		fd对应channel指针
	if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
		perror("epoll_add error");
		fd2chan_[fd].reset();		//fd2chan_指向了存放若干channel对象的数组
	}
}

// 修改描述符状态
void Epoll::epoll_mod(SP_Channel request, int timeout) {
	if (timeout > 0)
		add_timer(request, timeout);
	int fd = request->getFd();
	if (!request->EqualAndUpdateLastEvents()) {	//返回值为bool值，判断最近发生的事件是否等于当前事件。
		struct epoll_event event;
		event.data.fd = fd;
		event.events = request->getEvents();		//获取当前channel内的事件（调用epoll_wait装进去的）
		if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
			perror("epoll_mod error");
			fd2chan_[fd].reset();
		}
	}
}

// 从epoll中删除描述符
void Epoll::epoll_del(SP_Channel request)
{
	int fd = request->getFd();
	struct epoll_event event;
	event.data.fd = fd;
	event.events = request->getLastEvents();
	//event.events = 0;
	// request->EqualAndUpdateLastEvents();
	if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
		perror("epoll_del error");
	}
	fd2chan_[fd].reset();		//智能指针的reset来释放channel对象
	fd2http_[fd].reset();
}


// 返回活跃事件数    重点来了 其中一些重要变量如下：
//typedef std::shared_ptr<Channel> SP_Channel;
//返回值是用来存放智能指针的vector
//std::vector<epoll_event> events_;		存放活跃事件的结构体数组
std::vector<SP_Channel> Epoll::poll() {	
	while (true) {				//不停取，直到取到事件就退出
		int event_count = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);		//取到事件并放入events_数组中
		if (event_count < 0)
			perror("epoll wait error");
		std::vector<SP_Channel> req_data = getEventsRequest(event_count);
		if (req_data.size() > 0)
			return req_data;
	}
}

void Epoll::handleExpired() {
	timerManager_.handleExpiredEvent();
}

// 分发处理函数
std::vector<SP_Channel> Epoll::getEventsRequest(int events_num) {
	std::vector<SP_Channel> req_data;
	for (int i = 0; i < events_num; ++i) {
		// 获取有事件产生的描述符
		int fd = events_[i].data.fd;
		SP_Channel cur_req = fd2chan_[fd];		//从数组中拿到当前channel	每个channel对应一个fd，

		if (cur_req) {
			cur_req->setRevents(events_[i].events);	//revents   输出 ,  此处将活跃事件赋值给channel对象的revents_变量
			cur_req->setEvents(0);
			// 加入线程池之前将Timer和request分离
			//cur_req->seperateTimer();
			req_data.push_back(cur_req);
		}
		else {
			LOG << "SP cur_req is invalid";
		}
	}
	return req_data;
}

void Epoll::add_timer(SP_Channel request_data, int timeout) {
	shared_ptr<HttpData> t = request_data->getHolder();
	if (t)
		timerManager_.addTimer(t, timeout);
	else
		LOG << "timer add fail";
}