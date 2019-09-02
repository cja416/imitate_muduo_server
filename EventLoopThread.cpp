#include "EventLoopThread.h"
#include <functional>

EventLoopThread::EventLoopThread()
	:loop_(NULL),
	exiting_(false),
	thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),  //���̵߳ĺ���
	mutex_(),
	cond_(mutex_)
{}

EventLoopThread::~EventLoopThread() {		//�������˳�ѭ����join�߳�
	exiting_ = true;
	if (loop_ != NULL) {
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop() {		//��EventLoopThreadPool::start()����
	assert(!thread_.started());
	thread_.start();

	{
		MutexLockGuard lock(mutex_);		//�������һ���࣬�ں������ڴ�����ʱ�����󲢼��������˳�������ʱ�Զ����ö��������������н���
				// һֱ�ȵ�threadFun��Thread�������������������õ��̻߳�����threadFunc����������loop���������̣߳�
		while (loop_ == NULL)
			cond_.wait();		//�������������� ,	��������Ҳ��װΪһ�������࣬����this���а�����cond�Ķ���,һ���������̴߳�������˾ͱ�����
	}
	return loop_;
}


void EventLoopThread::threadFunc() {
	EventLoop loop;
	{
		MutexLockGuard lock(mutex_);
		loop_ = &loop;		//����	,������loop_�Ͳ�ΪNULL�ˣ������������ȵ���
		cond_.notify();
	}

	loop.loop();
	//assert(exiting_);
	loop_ = NULL;			//������һ��EventLoopThread����EventLoop�������¼�ѭ��������
}
