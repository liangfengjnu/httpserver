#include "eventloopthread.h"
#include <functional>
#include <assert.h>
EventloopThread::EventloopThread()
    : loop_(NULL),
      exiting_(false),
      thread_(std::bind(&EventloopThread::threadFunc, this), "EventloopThread"),
      mutex_(),
      cond_(mutex_) {}

EventloopThread::~EventloopThread() {
	exiting_ = true;
	if (loop_ != NULL) {
		loop_->quit();
		thread_.join();
	}
}

Eventloop* EventloopThread::startLoop() {
	assert(!thread_.started());
	thread_.start();
	{
		MutexLockGuard lock(mutex_);
		// 一直等到threadFun在Thread里真正跑起来
		while (loop_ == NULL) cond_.wait();
	}
	return loop_;
}

void EventloopThread::threadFunc() {
	Eventloop loop;

  {
		MutexLockGuard lock(mutex_);
		loop_ = &loop;
		cond_.notify();
  }

	loop.loop();
	// assert(exiting_);
	loop_ = NULL;
}