#include "eventloop.h"

Eventloop::eventloop() : channel_(this)
{
	
}

Eventloop::~Eventloop()
{
	
}

void Eventloop::loop()
{
	while(true)
	{
		int eventCount = epoller_->wait();
		for(int i = 0; i < eventCount; ++i)
		{
			
		}
	}
}