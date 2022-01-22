#include "epoller.h"
#include "channel.h"
#include <iostream>

Epoller::Epoller(Eventloop* loop) : 
events_(512),
loop_(loop)
{
	epollFd_ =  epoll_create(512);
}

Epoller::~Epoller()
{
	close(epollFd_);
}

void Epoller::poll(std::vector<Channel*>* channelList)
{
	int eventNums = epoll_wait(epollFd_, &*events_.begin(),
						 static_cast<int>(events_.size()),
						 0);
	int savedErrno = errno;
	
	if(eventNums > 0)
	{
		fillChannelList(eventNums, channelList);
	}

}

void Epoller::fillChannelList(int eventNums, std::vector<Channel*>* channelList)
{

	for(int i = 0; i < eventNums; ++i)
	{
		int fd =  events_[i].data.fd;
		Channel* channel = channels_[fd];
		channel->setRevents(events_[i].events);
		channelList->push_back(channel);
	}
	
}

int Epoller::setNonBlocking(int fd){
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void Epoller::epollAdd(Channel* channel)
{
	struct epoll_event ev = {0};
	int fd = channel->getFd();
	setNonBlocking(fd);
	ev.data.fd = fd;
	channels_[fd] = channel;
	ev.events = channel->getEvents();
	epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

void Epoller::updateChannel(Channel* channel)
{

    // update existing one with EPOLL_CTL_MOD/DEL
    if (channel->isNoneEvent())
    {
		epollDelete(channel);
    }
    else
    {
		epollMod(channel);
    }
}

void Epoller::removeChannel(Channel* channel)
{
	int fd = channel->getFd();
	epollDelete(channel);
	channels_.erase(fd);
}

void Epoller::epollDelete(Channel* channel)
{
	struct epoll_event ev = {0};
	int fd = channel->getFd();
	ev.data.fd = fd;
	channels_[fd] = channel;
	ev.events = channel->getEvents();
	epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

void Epoller::epollMod(Channel* channel)
{
	struct epoll_event ev = {0};
	int fd = channel->getFd();
	ev.data.fd = fd;
	channels_[fd] = channel;
	ev.events = channel->getEvents();
	epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}