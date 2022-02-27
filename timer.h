//#ifndef TIMER_H
//#define TIMER_H
#pragma once
#include <unistd.h>
#include <deque>
#include <memory>
#include <queue>
#include "httpconn.h"
#include "mutexlock.h"
#include "noncopyable.h"


class HttpConn;

class TimerNode {
public:
	TimerNode(std::shared_ptr<HttpConn> requestData, int timeout);
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
	std::shared_ptr<HttpConn> SPHttpConn;
};

struct TimerCmp 
{
	bool operator()(std::shared_ptr<TimerNode> &a,
                  std::shared_ptr<TimerNode> &b) const 
	{
		return a->getExpTime() > b->getExpTime();
	}
};

class TimerManager {
public:
	TimerManager();
	~TimerManager();
	void addTimer(std::shared_ptr<HttpConn> SPHttpConn, int timeout);
	void handleExpiredEvent();

private:
	typedef std::shared_ptr<TimerNode> SPTimerNode;
	std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp> timerNodeQueue;
  // MutexLock lock;
};

//#endif