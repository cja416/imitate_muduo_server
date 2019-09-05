#include "EventLoop.h"
#include "Logging.h"
#include "Util.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <iostream>
using namespace std;

__thread EventLoop* t_loopInThisThread = 0;  
 //EventLoop* t_loopInThisThread = 0;				��ȷ������__thread����  ��ʾ��io�߳�ӵ�е�eventloop��һһ��Ӧ��

int creatEventfd() {
	int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		LOG << "Failed in eventfd";
		abort();
	}
	return evtfd;
}


EventLoop::EventLoop()			//���캯��
:	looping_(false),
	poller_(new Epoll()),					//������½�epoll��socket������
	wakeupFd_(creatEventfd()),				//�����½��¼����ѵ��ļ�������
	quit_(false),
	eventHandling_(false),
	callingPendingFunctors_(false),
	threadId_(CurrentThread::tid()),
	pwakeupChannel_(new Channel(this,wakeupFd_))		//��channel�󶨵��ǻ���������
{
	if (t_loopInThisThread) {
		//LOG << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
	}
	else {
		t_loopInThisThread = this;
	}

	pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);		//���û��ѵ�channel�¼�
	pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead,this));
	pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
	poller_->epoll_add(pwakeupChannel_, 0);					//�������ѵ�eventfd�ҵ��������
}
	
void EventLoop::handleConn() {
	//poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET | EPOLLONESHOT), 0);ԭʼд��
	updatePoller(pwakeupChannel_, 0);
}

EventLoop::~EventLoop() {
	//wakeupChannel_->disableAll();
	//wakeupChannel_->remove();
	close(wakeupFd_);
	t_loopInThisThread = nullptr;
}

void EventLoop::wakeup() {				//����ɶʱ���ѵ�
	uint64_t one = 1;
	ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);//д1�������߳�
	if (n != sizeof one) {
		LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
	}
}

void EventLoop::handleRead() {			//����IO�̵߳�pwakeupChannel_�󶨵Ĵ�����
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
	if (isInLoopThread())					//����ж��Ƿ���IO�߳��У�
		cb();
	else
		queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor&& cb) {
	{
		MutexLockGuard lock(mutex_);				//���߳���IO�߳�����IO�̵߳�eventloop����,��֤�Ž�������е�ʱ��IO�̲߳������ڴ�������
		pendingFunctors_.emplace_back(std::move(cb));
	}

	if (!isInLoopThread() || callingPendingFunctors_)			//����IO�̷ַ߳�δ���¼�ʱ��������һ�³��򿴿�
		wakeup();
}

void EventLoop::loop() {
	assert(!looping_);		//ȷ���߳��е�eventloopδ����
	assert(isInLoopThread());	//ȷ��eventloop�Ѱ��߳�
	looping_ = true;
	quit_ = false;
	//LOG << "EventLoop " << this << " start looping";
	std::vector<SP_Channel> ret;		//SP��shared_ptr
	while (!quit_) {					//����ѭ��ȡ��eventloop��epoll��õ��¼�
		//cout << "doing" << endl;
		ret.clear();
		ret = poller_->poll();		//��ô����¼���Ϣ��channelָ������
		eventHandling_ = true;
		for (auto &it : ret)
			it->handleEvents();		//�߳��������,��poll�а�epoll_wait�õ����¼�������ret������channel�����revent���������ó�������
		eventHandling_ = false;								//ÿ��channel���˻ص��������磬accpetchannel���� Server::handNewConn()
		doPendingFunctors();								//��ÿ��IO�̵߳�channel���ǰ���HttpData::handleRead()����HttpData::handleWrite
		poller_->handleExpired();//���������Epoll���еĺ�����ÿ��IOѭ���д���ʱ����
	}
	looping_ = false;
}

void EventLoop::doPendingFunctors() {
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;
	{
		MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);		//pendingFunctors_   δ���¼�,��Ϊ����дfd�ҵ�io�̵߳�epoll��
	}
	
	for (size_t i = 0; i < functors.size(); ++i)
		functors[i]();							//��һ����δ������      ���Ĳ����ǵ�����runInLoopʱ��eventloopδռ��һ���߳�
																		//(����˵���̱߳�����������epoll�ϣ�Ӧ�ý�������)  IO�߳���������ʱ���յ��ķַ��¼��ݴ�������
	callingPendingFunctors_ = false;
}

void EventLoop::quit() {
	quit_ = true;
	if (!isInLoopThread()) {
		wakeup();
	}
}
