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
	:epollFd_(epoll_create1(EPOLL_CLOEXEC)),			//�л��߳�ʱ���Զ��رձ�epoll_fd
	events_(EVENTSNUM)
{
	assert(epollFd_ > 0);
}

Epoll::~Epoll() 
{}

//ע����������
void Epoll::epoll_add(SP_Channel request, int timeout) {
	int fd = request->getFd();
	if (timeout > 0) {
		add_timer(request, timeout);		//��ʱ��
		fd2http_[fd] = request->getHolder();		//fd2http_ָ���˴������HttpData��������� 
	}
	struct epoll_event event;
	event.data.fd = fd;				//������ע���ļ����������й���Ϣ
	event.events = request->getEvents();		//channel���Է��صõ���events

	request->EqualAndUpdateLastEvents();		//��������������¼��������¼���Ϊ����������¼�)

	fd2chan_[fd] = request;		//��Ȼ����fd��Ϊ��ȡ������Ϊ�β���map��		fd��Ӧchannelָ��
	if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
		perror("epoll_add error");
		fd2chan_[fd].reset();		//fd2chan_ָ���˴������channel���������
	}
}

// �޸�������״̬
void Epoll::epoll_mod(SP_Channel request, int timeout) {
	if (timeout > 0)
		add_timer(request, timeout);
	int fd = request->getFd();
	if (!request->EqualAndUpdateLastEvents()) {	//����ֵΪboolֵ���ж�����������¼��Ƿ���ڵ�ǰ�¼���
		struct epoll_event event;
		event.data.fd = fd;
		event.events = request->getEvents();		//��ȡ��ǰchannel�ڵ��¼�������epoll_waitװ��ȥ�ģ�
		if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
			perror("epoll_mod error");
			fd2chan_[fd].reset();
		}
	}
}

// ��epoll��ɾ��������
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
	fd2chan_[fd].reset();		//����ָ���reset���ͷ�channel����
	fd2http_[fd].reset();
}


// ���ػ�Ծ�¼���    �ص����� ����һЩ��Ҫ�������£�
//typedef std::shared_ptr<Channel> SP_Channel;
//����ֵ�������������ָ���vector
//std::vector<epoll_event> events_;		��Ż�Ծ�¼��Ľṹ������
std::vector<SP_Channel> Epoll::poll() {	
	while (true) {				//��ͣȡ��ֱ��ȡ���¼����˳�
		int event_count = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);		//ȡ���¼�������events_������
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

// �ַ�������
std::vector<SP_Channel> Epoll::getEventsRequest(int events_num) {
	std::vector<SP_Channel> req_data;
	for (int i = 0; i < events_num; ++i) {
		// ��ȡ���¼�������������
		int fd = events_[i].data.fd;
		SP_Channel cur_req = fd2chan_[fd];		//���������õ���ǰchannel	ÿ��channel��Ӧһ��fd��

		if (cur_req) {
			cur_req->setRevents(events_[i].events);	//revents   ��� ,  �˴�����Ծ�¼���ֵ��channel�����revents_����
			cur_req->setEvents(0);
			// �����̳߳�֮ǰ��Timer��request����
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