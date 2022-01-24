#include "channel.h"
#include "eventloop.h"

Channel::Channel(Eventloop* loop) 
:loop_(loop),
 events_(0),
 revents_(0),
 fd_(0)
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
	handleConn();
}

void Channel::update()
{
  //addedToLoop_ = true;
  loop_->updateToChannel(this);
}

void Channel::remove()
{
	loop_->removeChannel(this);
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

void Channel::handleConn() {
	if(connHandler_) 
	{
		connHandler_();
	}
}