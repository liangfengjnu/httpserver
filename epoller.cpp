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

void Epoller::poll(std::vector<std::shared_ptr<Channel>> &channelList)
{

	int eventNums = epoll_wait(epollFd_, &*events_.begin(),
						 static_cast<int>(events_.size()),
						 0);
	int savedErrno = errno;
	
	if(eventNums < 0) { printf("epoll wait error\n"); }
	else 
		fillChannelList(eventNums, channelList);

}

void Epoller::fillChannelList(int eventNums, std::vector<std::shared_ptr<Channel>> &channelList)
{
	for(int i = 0; i < eventNums; ++i)
	{
		int fd =  events_[i].data.fd;
		std::shared_ptr<Channel> channel = channels_[fd];
		if(channel)
		{
			channel->setRevents(events_[i].events);
			channel->setEvents(0);
			channelList.push_back(channel);
		}
		else{ printf("channel point is invalid\n");}
	}
}

void Epoller::updateChannel(std::shared_ptr<Channel> channel)
{

    // update existing one with EPOLL_CTL_MOD/DEL
    if (channel->isNoneEvent())
    {
		epoll_del(channel);
    }
    else
    {
		epoll_mod(channel);
    }
}

void Epoller::removeChannel(std::shared_ptr<Channel> channel)
{
	int fd = channel->getFd();
	epoll_del(channel);
	channels_.erase(fd);
}

void Epoller::epollAdd(std::shared_ptr<Channel> channel, int timeout)
{
	struct epoll_event ev = {0};
	int fd = channel->getFd();
	if(timeout > 0 )
	{
		printf("timeout > 0\n");
	}
	ev.data.fd = fd;
	channels_[fd] = channel;
	ev.events = channel->getEvents();
	channel->equalAndUpdateLastEvents();
	if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0)
	{
		printf("epollAdd error\n");
	}
}

void Epoller::epoll_del(std::shared_ptr<Channel> channel)
{
	struct epoll_event ev = {0};
	int fd = channel->getFd();
	ev.data.fd = fd;
	channels_[fd] = channel;
	ev.events = channel->getEvents();
	if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev) < 0)
	{
		printf("epoll_del error\n");
	}
}

void Epoller::epoll_mod(std::shared_ptr<Channel> channel)
{
	struct epoll_event ev = {0};
	int fd = channel->getFd();
	if(!channel->equalAndUpdateLastEvents())
	{
		ev.data.fd = fd;
		ev.events = channel->getEvents();
		if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) < 0)
		{	
			printf("epoll_mod error\n");
		}
	}
}