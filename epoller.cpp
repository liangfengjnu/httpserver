#include "epoller.h"

Epoller::Epoller(int maxEvent) : events_(maxEvent)
{
	epollFd_ =  epoll_create(512);
	
}

Epoller::~Epoller()
{
	close(epolleFd_);
}