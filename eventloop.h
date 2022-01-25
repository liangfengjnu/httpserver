#ifndef EVENTLOOP_H
#define EVENTLOOP_H


#include <memory>
#include <vector>



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
	void addToPoller(std::shared_ptr<Channel> channel, int timeout);
	void updateToChannel(std::shared_ptr<Channel> channel);
	void removeChannel(std::shared_ptr<Channel> channel);

private:
	//bool looping_;
	//int wakeupFd_;
	//bool isquit_;
	//bool eventHandling_;
	Epoller* epoller_;
	//mutable MutexLock mutex_;
	//const pid_t threadId_;
	
	
	//shared_ptr<Channel> pwakeupChannel_;
	std::vector<std::shared_ptr<Channel>> channelList_;
	
	//std::vector<Functor> pendingFunctors_;
	//bool callingPendingFunctors_;
	
	// void wakeup();
	// void handleRead();
	// void doPendingFunctors();
	// void handleConn();
};

#endif