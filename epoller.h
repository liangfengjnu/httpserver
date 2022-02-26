#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "eventloop.h"
#include "timer.h"


class Channel;
class Epoller
{
public:
	Epoller(Eventloop* loop);
	~Epoller();

	void poll(std::vector<std::shared_ptr<Channel>> &channelList);
	void fillChannelList(int events, std::vector<std::shared_ptr<Channel>>& channelList);
	
	void epoll_add(std::shared_ptr<Channel> channel, int timeout);
	void epoll_mod(std::shared_ptr<Channel> channel, int timeout);
	void epoll_del(std::shared_ptr<Channel> channel);
	void addTimer(std::shared_ptr<Channel> channel, int timeout);
	void getEpollFd() {return epollFd_;}
	void handleExpired();
	
private:
	int epollFd_;
	std::vector<struct epoll_event> events_;
	std::shared_ptr<Channel> channels_[512];
	TimerManager timerManager_;
	Eventloop* loop_;

};

#endif