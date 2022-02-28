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
		
		//std::shared_ptr<Channel> channel = (events_[i].data.ptr);
		int fd =  events_[i].data.fd;	
		std::shared_ptr<Channel> curChannel = channels_[fd];
		if(curChannel)
		{
			curChannel->setRevents(events_[i].events);
			//curChannel->setEvents(0);
			channelList.push_back(curChannel);
		}
		else
		{
			printf("curChannel is invalid\n");
		}	
	}
}

void Epoller::epoll_add(std::shared_ptr<Channel> channel, int timeout)
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
	if(epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) < 0)
	{
		printf("epollAdd error\n");
		channels_[fd].reset();
	}
}


void Epoller::epoll_del(std::shared_ptr<Channel> channel)
{
	struct epoll_event ev = {0};
	int fd = channel->getFd();
	ev.data.fd = fd;
	ev.events = channel->getEvents();
	if(epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev) < 0)
	{
		printf("epoll_del error\n");
	}
	channels_[fd].reset();
}

void Epoller::epoll_mod(std::shared_ptr<Channel> channel, int timeout)
{
	if(timeout > 0) addTimer(channel, timeout);
	struct epoll_event ev = {0};
	int fd = channel->getFd();
		ev.data.fd = fd;
		ev.events = channel->getEvents();
		if(epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) < 0)
		{	
			printf("epoll_mod error\n");
			channels_[fd].reset();
		}
}

void Epoller::handleExpired()
{
	timerManager_.handleExpiredEvent();
}

std::vector<std::shared_ptr<Channel>> Epoller::getEventsRequest(int events_num) 
{
	std::vector<std::shared_ptr<Channel> > req_data;
	for (int i = 0; i < events_num; ++i) 
	{
		// 获取有事件产生的描述符
		int fd = events_[i].data.fd;

		std::shared_ptr<Channel> cur_req = channels_[fd];

		if (cur_req) 
		{
			cur_req->setRevents(events_[i].events);
			cur_req->setEvents(0);
			// 加入线程池之前将Timer和request分离
			// cur_req->seperateTimer();
			req_data.push_back(cur_req);
		}
		else 
		{
			//LOG << "SP cur_req is invalid";
			printf("cur_req is invalid!\n");
		}
	}
	return req_data;
}

void Epoller::addTimer(std::shared_ptr<Channel> request_data, int timeout) 
{
	std::shared_ptr<HttpConn> t = request_data->getHolder();
	if (t)
		timerManager_.addTimer(t, timeout);
	else
		printf("timer add fail!\n");
		//LOG << "timer add fail";
}