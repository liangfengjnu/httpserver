#include "channel.h"
#include "eventloop.h"
#include "Util.h"


Channel::Channel(Eventloop* loop) 
:loop_(loop),
 events_(0),
 revents_(0),
 lastEvents_(0),
 fd_(0)
{
	
}

Channel::Channel(Eventloop* loop, int fd) 
:loop_(loop),
 fd_(fd),
 events_(0),
 revents_(0),
 lastEvents_(0)
{
	
}

Channel::~Channel()
{
	
}

void Channel::handleEvents()
{
	events_ = 0;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
		events_ = 0;
		return;
    }
    if (revents_ & EPOLLERR) {
		if (errorHandler_) errorHandler_();
		events_ = 0;
		return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) 
	{
		if(readHandler_) 
		{
			readHandler_();
		}
    }
    if (revents_ & EPOLLOUT) 
	{
		if(writeHandler_) 
		{
			writeHandler_();
		}
    }
}


void Channel::handleRead() {
	if(readHandler_) 
	{
		readHandler_();
	}
}	

void Channel::handleWrite() {
	if(writeHandler_) 
	{
		writeHandler_();
	}
}
