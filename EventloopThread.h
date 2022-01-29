#include "eventloop.h"
#include "condition.h"
#include "mutexlock.h"
#include "thread.h"
#include "noncopyable.h"


class EventloopThread : noncopyable
{
public:
	EventloopThread();
	~EventloopThread();
	Eventloop* startLoop();

private:
	void threadFunc();
	Eventloop* loop_;
	bool exiting_;
	Thread thread_;
	MutexLock mutex_;
	Condition cond_;
};