#include "Eventloop.h"
#include "base/Condition.h"
#include "base/MutexLock.h"
#include "base/Thread.h"
//#include "base/noncopyable.h"


class EventloopThread //: noncopyable
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