#include "eventloop.h"
#include "epoller.h"
#include "channel.h"
#include <iostream>

Eventloop::Eventloop()
:epoller_(new Epoller(this))
{
	
}

Eventloop::~Eventloop()
{
	
}

void Eventloop::loop()
{
	while(true)
	{
		//int eventCount = 
		epoller_->poll(&channelList_);
		for(Channel* channel : channelList_)
		{
			activeChannel_ = channel;
			activeChannel_->handleEvents();
		}
	}
}

void Eventloop::addToPoller(std::shared_ptr<Channel> channel)
{
	epoller_->epollAdd(channel);
}