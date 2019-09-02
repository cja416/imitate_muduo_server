#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout)		//timeNode保存的是一个HttpData请求业务的时间信息
	:deleted_(false),
	SPHttpData(requestData)
{
	struct timeval now;
	gettimeofday(&now, NULL);					//此函数不是系统调用，而是在用户态实现的，没有上下文切换和陷入内核的开销
	//以毫秒计
	expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;		//设置每个TimerNode的超时时间=当前时间+timeout
}


/*
struct timeval {
time_t tv_sec;	// seconds
long tv_usec;	// microseconds
};
获取当前时间用gettimeofday(&now, NULL);
*/

TimerNode::~TimerNode() {
	if (SPHttpData)
		SPHttpData->handleClose();
}

void TimerNode::update(int timeout) {
	struct timeval now;
	gettimeofday(&now, NULL);
	expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid() {
	struct timeval now;
	gettimeofday(&now, NULL);
	size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
	if (temp < expiredTime_)				//还未到达超时时间，就算有效
		return true;
	else {
		this->setDeleted();
		return false;
	}
}

void TimerNode::clearReq() {		//清除请求		HttpData::reset()、HttpData::seperateTimer()会做这件事，会将timernode的isdeleted_置为1
	SPHttpData.reset();
	this->setDeleted();
}

TimerManager::TimerManager()
{ }

TimerManager::~TimerManager()
{ }

void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout) {
	SPTimerNode new_node(new TimerNode(SPHttpData, timeout));			//timeNode保存的是一个HttpData请求业务的时间信息
	timerNodeQueue.push(new_node);		//一个最大堆,存放了一系列指向TimerNode的指针
	SPHttpData->linkTimer(new_node);
}


void TimerManager::handleExpiredEvent() {
	//MutexLockGuard locker(lock);
	while (!timerNodeQueue.empty()) {						//将堆顶所有isdeleted和isvalid==false的timernode都pop。
		SPTimerNode ptimer_now = timerNodeQueue.top();				//智能指针管理这个timernode，当一次循环结束时，引用计数减为0，就会调用它的析构函数
		if (ptimer_now->isDeleted())
			timerNodeQueue.pop();
		else if (ptimer_now->isValid() == false)
			timerNodeQueue.pop();
		else
			break;		//堆顶元素是定时时间最长的那个TimerNode，就是距离上次处理业务的时间最长的那个
	}
}						