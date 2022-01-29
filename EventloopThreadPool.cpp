#include "eventloopthreadpool.h"

EventloopThreadPool::EventloopThreadPool(Eventloop *baseLoop, int numThreads)
:   baseLoop_(baseLoop),
	started_(false),
	numThreads_(numThreads),
	next_(0) 
{
	if (numThreads_ <= 0) {
		printf("numThreads_ <= 0\n");
		abort();
	}
}

void EventloopThreadPool::start() {
	baseLoop_->assertInLoopThread();
	started_ = true;
	for (int i = 0; i < numThreads_; ++i) 
	{
		std::shared_ptr<EventloopThread> t(new EventloopThread());
		threads_.push_back(t);
		loops_.push_back(t->startLoop());
	}
}

Eventloop *EventloopThreadPool::getNextLoop() {
	baseLoop_->assertInLoopThread();
	assert(started_);
	Eventloop *loop = baseLoop_;
	if (!loops_.empty()) 
	{
		loop = loops_[next_];
		next_ = (next_ + 1) % numThreads_;
	}
	return loop;
}