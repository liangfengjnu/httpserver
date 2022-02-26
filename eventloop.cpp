#include <iostream>
#include <sys/eventfd.h>
#include <sys/epoll.h>

#include "Util.h"
#include "eventloop.h"
#include "epoller.h"
#include "channel.h"



__thread Eventloop* t_loopInThisThread = 0;

int createEventfd()
{
	int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		printf("Failed in eventfd\n");
		abort();
	}
	return evtfd;
}

Eventloop::Eventloop():
	looping_(false),
	wakeupFd_(createEventfd()),
	quit_(false),
	epoller_(new Epoller(this)),
	eventHandling_(false),
	callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
	pwakeupChannel_(new Channel(this)) 
{
	if (t_loopInThisThread) 
	{
		// LOG << "Another Eventloop " << t_loopInThisThread << " exists in this
		// thread " << threadId_;
	} 
	else 	
	{
		t_loopInThisThread = this;
	}
	pwakeupChannel_->setFd(wakeupFd_);
	pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
	pwakeupChannel_->setReadHandler(std::bind(&Eventloop::handleRead, this));
	epoller_->epoll_add(pwakeupChannel_, 0);
}

Eventloop::~Eventloop()
{
	close(wakeupFd_);
	t_loopInThisThread = NULL;
}

void Eventloop::handleConn()
{
	updateToChannel(pwakeupChannel_, 0);
}

void Eventloop::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);
  if (n != sizeof one) 
  {
	  printf("Eventloop::wakeup() writes %d bytes instead of 8\n", n);
    //LOG << "Eventloop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void Eventloop::handleRead()
{
	uint64_t one = 1;
	ssize_t n = readn(wakeupFd_, &one, sizeof one);
	if (n != sizeof one)
	{
		printf("Eventloop::handleRead() reads %d bytes instead of 8\n", n);
	}
	pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

void Eventloop::runInLoop(std::function<void()>&& cb) 
{
	if (isInLoopThread())
		cb();
	else
		queueInLoop(std::move(cb));
}


void Eventloop::queueInLoop(std::function<void()>&& cb) 
{
	{
		MutexLockGuard lock(mutex_);
		pendingFunctors_.emplace_back(std::move(cb));
	}

	if (!isInLoopThread() || callingPendingFunctors_) wakeup();
}


void Eventloop::loop()
{
	assert(!looping_);
	assert(isInLoopThread());
	looping_ = true;
	quit_ = false;
	while(!quit_)
	{
		channelList_.clear();
		epoller_->poll(channelList_);
		eventHandling_ = true;
		if(channelList_.size() > 0 )
			printf("the size of channelList_ is %d\n", (int)channelList_.size());
		for(auto& channel : channelList_)
		{
			channel->handleEvents();
		}
		eventHandling_ = false;
		doPendingFunctors();
		//poller_->handleExpired();
	}
	looping_ = false;
}

void Eventloop::addToPoller(std::shared_ptr<Channel> channel, int timeout = 0)
{
	epoller_->epoll_add(channel, timeout);
}


void Eventloop::updateToChannel(std::shared_ptr<Channel> channel)
{
	epoller_->epoll_mod(channel);
}

void Eventloop::removeChannel(std::shared_ptr<Channel> channel)
{
	epoller_->epoll_del(channel);
}

void Eventloop::doPendingFunctors() 
{
	std::vector<std::function<void()>> functors;
	callingPendingFunctors_ = true;

	{
		MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);
	}

	for (size_t i = 0; i < functors.size(); ++i) 
		functors[i]();
	callingPendingFunctors_ = false;
}


void Eventloop::quit() 
{
	quit_ = true;
	if (!isInLoopThread())
	{
		wakeup();
	}
}