#include "epoller.h"

Epoller::Epoller(int maxEvent) : events_(maxEvent)
{
	epollFd_ =  epoll_create(512);
}

Epoller::~Epoller()
{
	close(epolleFd_);
}

void Epoller::wait(std::vector<Channel*> channelList)
{
	int eventNums = epoll_wait(epollFd_, &*events_.begin(),
						 static_cast<int>(events_.size(),
						 NULL);
	int savedErrno = errno;
	
	if(eventNums > 0)
	{
		fillChannelList(eventNums, channelList);
	}
}

void Epoller::fillChannelList(int eventNums, std::vector<Channel*> channelList)
{
	for(int i = 0; i < eventNums; ++i)
	{
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		int fd = channel->getFd();
		std::vector<Channel*>::const_iterator it = channels_.find(fd);
		assert(it != channels_.end());
		assert(it->second == channel);
		
		channel->setRevents(events_[i].events);
		channelList->push_back(channel);
	}
}