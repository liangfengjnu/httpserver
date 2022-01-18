#ifndef EVENTLOOP_H
#define EVENTLOOP_H

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
};

#endif