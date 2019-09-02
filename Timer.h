#pragma once
#include "HttpData.h"
#include "noncopyable.h"
#include "MutexLock.h"
#include <unistd.h>
#include <memory>
#include <queue>
#include <deque>

class HttpData;

class TimerNode
{
public:
	TimerNode(std::shared_ptr<HttpData> requestData, int timeout);
	~TimerNode();
	TimerNode(TimerNode &tn);
	void update(int timeout);
	bool isValid();
	void clearReq();
	void setDeleted() { deleted_ = true; }
	bool isDeleted() const { return deleted_; }
	size_t getExpTime() const { return expiredTime_; }

private:
	bool deleted_;
	size_t expiredTime_;
	std::shared_ptr<HttpData> SPHttpData;				//当指向httpdata的timernode被析构后，httpdata的引用计数为0，会调用析构函数关闭fd
};

struct TimerCmp
{
	bool operator()(std::shared_ptr<TimerNode> &a, std::shared_ptr<TimerNode> &b) const
	{
		return a->getExpTime() > b->getExpTime();			//已经经历的时间最大的放在前面，其实是一个最大堆
	}
};

class TimerManager
{
public:
	TimerManager();
	~TimerManager();
	void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
	void handleExpiredEvent();

private:
	typedef std::shared_ptr<TimerNode> SPTimerNode;
	std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerNodeQueue;
	//MutexLock lock;
};