#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>

class Eventloop;

class Channel
{
public:
	Channel(Eventloop* loop);
	~Channel();
	

private:
	typedf std::function<void()> callBack;
	Eventloop* loop_;
};

#endif