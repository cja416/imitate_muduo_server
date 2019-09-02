#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
	:baseLoop_(baseLoop),
	started_(false),
	numThreads_(numThreads),
	next_(0)
{
	if (numThreads_ <= 0)
	{
		LOG << "numThreads_ <= 0";
		abort();
	}
}

void EventLoopThreadPool::start() {			//��Server::start()�е��ÿ�ʼ
	baseLoop_->assertInLoopThread();		//�ж��Ƿ���ѭ���߳���
	started_ = true;
	for (int i = 0; i < numThreads_; ++i) {
		std::shared_ptr<EventLoopThread> t(new EventLoopThread());	//���ݴ�����߳�������ָ��loop�̵߳�����ָ��
		threads_.push_back(t);		//������ѭ���̵߳�ָ�������Ѵ����̵߳����飬����ͳһ����
		loops_.push_back(t->startLoop());	//�����½��̵߳�ѭ�������������ѭ����������
	}
}

EventLoop* EventLoopThreadPool::getNextLoop() {
	baseLoop_->assertInLoopThread();
	assert(started_);
	EventLoop* loop = baseLoop_;	//��ʱ������ֵΪ���߳�
	if (!loops_.empty())		//�ж��������ѭ���߳���������û�� ����ѭ�����߳�
	{
		loop = loops_[next_];
		next_ = (next_ + 1) % numThreads_;			//�������νround robin�㷨
	}
	return loop;		//ȡ����һ��IO�߳�loop
}