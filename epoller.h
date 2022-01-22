#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>
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

	void poll(std::vector<Channel*>* channelList);
	void fillChannelList(int events, std::vector<Channel*>* channelList);
	
	int setNonBlocking(int fd);
	void epollAdd(Channel* channel);
	void updateChannel(Channel* channel);
	void removeChannel(Channel* channel);
	void epollDelete(Channel* channel);
	void epollMod(Channel* channel);
private:
	int epollFd_;
	std::vector<struct epoll_event> events_;
	std::map<int, Channel*> channels_;
	Eventloop* loop_;
};

#endif