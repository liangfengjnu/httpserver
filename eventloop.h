#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>
#include <memory>

//#include "base/CurrentThread.h"

class Epoller;
class Channel;

class Eventloop
{
public:
	Eventloop();
	~Eventloop();
	
	
	//bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
	void loop();
	void addToPoller(Channel* channel);
	void updateToChannel(Channel* channel);
	void removeChannel(Channel* channel);

private:
	//bool looping_;
	//int wakeupFd_;
	//bool isquit_;
	//bool eventHandling_;
	Epoller* epoller_;
	//mutable MutexLock mutex_;
	//const pid_t threadId_;
	
	
	//shared_ptr<Channel> pwakeupChannel_;
	
	std::vector<Channel*> channelList_;
	Channel* activeChannel_;
	
	//std::vector<Functor> pendingFunctors_;
	//bool callingPendingFunctors_;
	
	// void wakeup();
	// void handleRead();
	// void doPendingFunctors();
	// void handleConn();
};

#endif