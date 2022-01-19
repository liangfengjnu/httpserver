#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>
#include <errno.h>
#include <sys/epoll.h>

#include "eventloop.h"

class Channel;
class Epoller
{
public:
	Epoller(int maxEvent);
	~Epoller();

	void wait(std::vector<Channel*>* channelList);
	void fillChannelList(int events, std::vector<Channel*>* channelList);
	
	void epollAdd(std::shared_ptr<Channel> channel);
private:
	int epollFd_;
	std::vector<epoll_event> events_;
	std::map<int, Channel*> channels_;
};

#endif