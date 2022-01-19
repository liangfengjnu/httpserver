#include "eventloop.h"
#include "epoller.h"
#include "channel.h"

Eventloop::Eventloop()
:epoller_(new Epoller(512))
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
		epoller_->wait(&channelList_);
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