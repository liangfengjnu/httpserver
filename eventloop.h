#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>

#include "channel.h"
#include "epoller.h"

class Eventloop
{
public:
	Eventloop();
	~Eventloop();
	
	void loop();

private:
	Channel* channel_;
	Epoller* epoller_;
	
	std::vector<Channel*> channelList_;
	Channel* activeChannel_;
};

#endif