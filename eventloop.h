#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <vector>
#include <memory>

class Epoller;
class Channel;

class Eventloop
{
public:
	Eventloop();
	~Eventloop();
	
	void loop();
	void addToPoller(std::shared_ptr<Channel> channel);
private:
	Channel* channel_;
	Epoller* epoller_;
	
	std::vector<Channel*> channelList_;
	Channel* activeChannel_;
};

#endif