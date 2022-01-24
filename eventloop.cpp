#include <iostream>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#include "eventloop.h"
#include "epoller.h"
#include "channel.h"


// int createEventfd()
// {
	// int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	// if (evtfd < 0)
	// {
		// printf("Failed in eventfd\n");
		// abort();
	// }
	// return evtfd;
// }

Eventloop::Eventloop():
	//looping_(false),
	//wakeupFd_(createEventfd()),
	//isquit_(false),
	epoller_(new Epoller(this))
	//eventHandling_(false),
   // callingPendingFunctors_(false),
    //threadId_(CurrentThread::tid()),
	//pwakeupChannel_(new Channel(this, wakeupFd_)) 
{
	// pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
	// pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));
	// pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));
	//epoller_->epoll_add(pwakeupChannel_);
}

Eventloop::~Eventloop()
{
	//close(wakeupFd_);
}

// void Eventloop::handleRead()
// {
	// uint64_t one = 1;
	// ssize_t n = readn(wakeupFd_, &one, sizeof one);
	// if (n != sizeof one) {
		// printf("EventLoop::handleRead() reads, %d, bytes instead of 8\n", n);
	// }
	// pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
// }

// void Eventloop::handleConn()
// {
	// updateToChannel(pwakeupChannel_);
// }

void Eventloop::loop()
{
	while(true)
	{
		//int eventCount = 
		epoller_->poll(&channelList_);
		//printf("the size of channelList_ is %d\n", (int)channelList_.size());
		for(Channel* channel : channelList_)
		{
			activeChannel_ = channel;
			activeChannel_->handleEvents();
		}
	}
}

void Eventloop::addToPoller(Channel* channel)
{
	epoller_->epollAdd(channel);
}


void Eventloop::updateToChannel(Channel* channel)
{
	epoller_->updateChannel(channel);
}

void Eventloop::removeChannel(Channel* channel)
{
	epoller_->removeChannel(channel);
}

// void Eventloop::doPendingFunctors() {
	// std::vector<Functor> functors;
	// callingPendingFunctors_ = true;

	// {
		// MutexLockGuard lock(mutex_);
		// functors.swap(pendingFunctors_);
	// }

	// for (size_t i = 0; i < functors.size(); ++i) functors[i]();
	// callingPendingFunctors_ = false;
// }


// void Eventloop::quit() {
	// quit_ = true;
	// if (!isInLoopThread()) {
		// wakeup();
	// }
// }