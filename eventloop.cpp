#include "eventloop.h"

Eventloop::eventloop()
:epoller_(new Epoller())
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
		epoller_->wait(channelList_);
		for(Channel* channel : channelList_)
		{
			activeChannel_ = channel;
			activeChannel_->handleEvent();
		}
	}
}

void Eventloop::addToPoller(std::shared_ptr<Channel> channel)
{
	epoller_->epollAdd(channel);
}