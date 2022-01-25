#include "timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

TimerNode::TimerNode(std::shared_ptr<HttpConn> requestData, int timeout)
    : deleted_(false), SPHttpConn(requestData) 
{
	struct timeval now;
	gettimeofday(&now, NULL);
	// 以毫秒计
	expiredTime_ =
      (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

TimerNode::~TimerNode() 
{
	if(SPHttpConn) 
		SPHttpConn->handleClose();
}

TimerNode::TimerNode(TimerNode &tn)
    : SPHttpConn(tn.SPHttpConn), expiredTime_(0)
{}

void TimerNode::update(int timeout)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

bool TimerNode::isValid() 
{
	struct timeval now;
	gettimeofday(&now, NULL);
	size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
	if (temp < expiredTime_)
		return true;
	else 
	{
		this->setDeleted();
		return false;
	}
}

void TimerNode::clearReq() 
{
  SPHttpConn.reset();
  this->setDeleted();
}

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::addTimer(std::shared_ptr<HttpConn> SPHttpConn, int timeout) 
{
	SPTimerNode new_node(new TimerNode(SPHttpConn, timeout));
	timerNodeQueue.push(new_node);
	SPHttpConn->linkTimer(new_node);
}


void TimerManager::handleExpiredEvent() 
{
	// MutexLockGuard locker(lock);
	while (!timerNodeQueue.empty()) 
	{
		SPTimerNode ptimer_now = timerNodeQueue.top();
		if (ptimer_now->isDeleted())
			timerNodeQueue.pop();
		else if (ptimer_now->isValid() == false)
			timerNodeQueue.pop();
		else
			break;
	}
}