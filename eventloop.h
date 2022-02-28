#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <assert.h>
#include <memory>
#include <vector>
#include "currentthread.h"
#include "thread.h"

class Epoller;
class Channel;


class Eventloop
{
public:
	Eventloop();
	~Eventloop();
	
	void runInLoop(std::function<void()>&& cb);
	void queueInLoop(std::function<void()>&& cb);
	
	bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
	void assertInLoopThread() { assert(isInLoopThread()); }
	void loop();
	void quit();
	void addToPoller(std::shared_ptr<Channel> channel, int timeout = 0);
	void updateToChannel(std::shared_ptr<Channel> channel, int timeout = 0);
	void removeChannel(std::shared_ptr<Channel> channel);

private:
	bool looping_;
	int wakeupFd_;
	bool quit_;
	bool eventHandling_;
	Epoller* epoller_;
	mutable MutexLock mutex_;
	
	
	std::shared_ptr<Channel> pwakeupChannel_;
	std::vector<std::shared_ptr<Channel>> channelList_;
	
	std::vector<std::function<void()>> pendingFunctors_;
	bool callingPendingFunctors_;
	
	const pid_t threadId_;
	
	void wakeup();
	void handleRead();
	void doPendingFunctors();
	void handleConn();
};

#endif