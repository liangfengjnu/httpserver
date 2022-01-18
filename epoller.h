#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>

class Epoller
{
public:
	Epoller(int maxEvent);
	~Epoller();

private:
	int epollFd_;
	std::vector<epoll_event> events_;
};

#endif