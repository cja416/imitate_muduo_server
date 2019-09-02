#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout)		//timeNode�������һ��HttpData����ҵ���ʱ����Ϣ
	:deleted_(false),
	SPHttpData(requestData)
{
	struct timeval now;
	gettimeofday(&now, NULL);					//�˺�������ϵͳ���ã��������û�̬ʵ�ֵģ�û���������л��������ں˵Ŀ���
	//�Ժ����
	expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;		//����ÿ��TimerNode�ĳ�ʱʱ��=��ǰʱ��+timeout
}


/*
struct timeval {
time_t tv_sec;	// seconds
long tv_usec;	// microseconds
};
��ȡ��ǰʱ����gettimeofday(&now, NULL);
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
	if (temp < expiredTime_)				//��δ���ﳬʱʱ�䣬������Ч
		return true;
	else {
		this->setDeleted();
		return false;
	}
}

void TimerNode::clearReq() {		//�������		HttpData::reset()��HttpData::seperateTimer()��������£��Ὣtimernode��isdeleted_��Ϊ1
	SPHttpData.reset();
	this->setDeleted();
}

TimerManager::TimerManager()
{ }

TimerManager::~TimerManager()
{ }

void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout) {
	SPTimerNode new_node(new TimerNode(SPHttpData, timeout));			//timeNode�������һ��HttpData����ҵ���ʱ����Ϣ
	timerNodeQueue.push(new_node);		//һ������,�����һϵ��ָ��TimerNode��ָ��
	SPHttpData->linkTimer(new_node);
}


void TimerManager::handleExpiredEvent() {
	//MutexLockGuard locker(lock);
	while (!timerNodeQueue.empty()) {						//���Ѷ�����isdeleted��isvalid==false��timernode��pop��
		SPTimerNode ptimer_now = timerNodeQueue.top();				//����ָ��������timernode����һ��ѭ������ʱ�����ü�����Ϊ0���ͻ����������������
		if (ptimer_now->isDeleted())
			timerNodeQueue.pop();
		else if (ptimer_now->isValid() == false)
			timerNodeQueue.pop();
		else
			break;		//�Ѷ�Ԫ���Ƕ�ʱʱ������Ǹ�TimerNode�����Ǿ����ϴδ���ҵ���ʱ������Ǹ�
	}
}						