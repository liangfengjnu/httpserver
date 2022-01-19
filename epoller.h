#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>
#include <errno.h>
#include <sys/epoll.h>

#include "channel.h"
#include "eventloop.h"

class Epoller
{
public:
	Epoller(int maxEvent);
	~Epoller();

	void wait(std::vector<Channel*> channelList);
	void fillChannelList(int events, std::vector<Channel*> channelList);
private:
	int epollFd_;
	std::vector<epoll_event> events_;
	std::map<int, Channel*> channels;
};

#endif