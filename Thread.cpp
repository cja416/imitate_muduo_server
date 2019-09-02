#include "Thread.h"
#include "CurrentThread.h"
#include <memory>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
using namespace std;


namespace CurrentThread {			//此namespace已在CurrentThread.h中声明，此处定义
	__thread int t_cachedTid = 0;	//	cacahed高速缓冲区存储的
	__thread char t_tidString[32];
	__thread int t_tidStringLength = 6;
	__thread const char* t_threadName = "default";
}

pid_t gettid() {
	return static_cast<pid_t>(::syscall(SYS_gettid));		//系统调用获取真实的线程tid
}

void CurrentThread::cacheTid() {
	if (t_cachedTid == 0) {
		t_cachedTid = gettid();
		t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);//将t_cachedTid的内容写入t_tidString中，返回值是写入的字节数
	}
}

// 为了在线程中保留name,tid这些数据,创建线程时用到这个结构体    结构体的默认访问权限是public的
struct ThreadData {
	typedef Thread::ThreadFunc ThreadFunc;
	ThreadFunc func_;
	string name_;
	pid_t* tid_;
	CountDownLatch* latch_;

	ThreadData(const ThreadFunc& func,const string& name,pid_t* tid, CountDownLatch *latch)
		:func_(func),
		name_(name),
		tid_(tid),
		latch_(latch)
	{}

	void runInThread() {
		*tid_ = CurrentThread::tid();
		tid_ = NULL;
		latch_->countDown();
		latch_ = NULL;

		CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
		prctl(PR_SET_NAME, CurrentThread::t_threadName);
		//第一个参数是操作类型，指定PR_SET_NAME，即设置进程名    第二个参数是进程名字符串，长度至多16字节

		func_();
		CurrentThread::t_threadName = "finish";
	}
};


void *startThread(void* obj) {
	ThreadData* data = static_cast<ThreadData*>(obj);
	data->runInThread();
	delete data;
	return NULL;
}

Thread::Thread(const ThreadFunc &func, const string &n)
	:started_(false),
	joined_(false),
	pthreadId_(0),
	tid_(0),
	func_(func),
	name_(n),
	latch_(1)
{
	setDefaultName();
}


Thread::~Thread() {
	if (started_ && !joined_)
		pthread_detach(pthreadId_);
}

void Thread::setDefaultName() {
	if (name_.empty()) {
		char buf[32];
		snprintf(buf, sizeof buf, "Thread");
		name_ = buf;
	}
}

void Thread::start() {
	assert(!started_);
	started_ = true;
	ThreadData* data = new ThreadData(func_, name_,&tid_, &latch_);
	if (pthread_create(&pthreadId_, NULL, &startThread, data)) {
		started_ = false;
		delete data;
	}
	else {							//注意，pthread_create创建成功是返回0的
		latch_.wait();
		assert(tid_ > 0);
	}
}

int Thread::join() {
	assert(started_);
	assert(!joined_);
	joined_ = true;
	return pthread_join(pthreadId_, NULL);
}