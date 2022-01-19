#include "epoller.h"
#include "channel.h"

Epoller::Epoller(int maxEvent) : events_(maxEvent)
{
	epollFd_ =  epoll_create(512);
}

Epoller::~Epoller()
{
	close(epollFd_);
}

void Epoller::wait(std::vector<Channel*>* channelList)
{
	int eventNums = epoll_wait(epollFd_, &*events_.begin(),
						 static_cast<int>(events_.size()),
						 NULL);
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
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		int fd = channel->getFd();
		std::map<int, Channel*>::const_iterator it = channels_.find(fd);
		assert(it != channels_.end());
		assert(it->second == channel);
		
		channel->setRevents(events_[i].events);
		channelList->push_back(channel);
	}
}

void Epoller::epollAdd(std::shared_ptr<Channel> channel)
{
	epoll_event ev = {0};
	ev.data.fd = channel->getFd();
	ev.events = channel->getEvents();
	epoll_ctl(epollFd_, EPOLL_CTL_ADD, channel->getFd(), &ev);
}