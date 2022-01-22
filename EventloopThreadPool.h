#include <memory>
#include <vector>
#include "EventloopThread.h"


class EventloopThreadPool
{
public:
	EventloopThreadPool(EventLoop* baseLoop, int numThreads);

	~EventloopThreadPool() { printf( "~EventloopThreadPool()\n)"; }
	void start();

	Eventloop* getNextLoop();

private:
	Eventloop* baseLoop_;
	bool started_;
	int numThreads_;
	int next_;
	std::vector<std::shared_ptr<EventloopThread>> threads_;
	std::vector<Eventloop*> loops_;
};