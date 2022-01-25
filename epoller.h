#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <unordered_map>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "eventloop.h"

class Channel;
class Epoller
{
public:
	Epoller(Eventloop* loop);
	~Epoller();

	void poll(std::vector<std::shared_ptr<Channel>> &channelList);
	void fillChannelList(int events, std::vector<std::shared_ptr<Channel>>& channelList);
	
	void epollAdd(std::shared_ptr<Channel> channel, int timeout);
	void updateChannel(std::shared_ptr<Channel> channel);
	void removeChannel(std::shared_ptr<Channel> channel);
	void epoll_del(std::shared_ptr<Channel> channel);
	void epoll_mod(std::shared_ptr<Channel> channel);
private:
	int epollFd_;
	std::vector<struct epoll_event> events_;
	std::unordered_map<int, std::shared_ptr<Channel>> channels_;
	Eventloop* loop_;
};

#endif