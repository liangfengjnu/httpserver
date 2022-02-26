#ifndef CHANNEL_H
#define CHANNEL_H


#include <functional>
#include <sys/epoll.h>
#include <memory>
#include "timer.h"


class Eventloop;
class HttpConn;

class Channel 
{
private:
	typedef std::function<void()> callBack;
public:
	Channel(Eventloop* loop);
	Channel(Eventloop* loop, int fd);
	~Channel();
	
	void setFd(int fd){fd_ = fd;}
	int getFd(){return fd_;}
	
	void setReadHandler(callBack&& readHandler){readHandler_ = readHandler;}
	void setWriteHandler(callBack&& writeHandler){writeHandler_ = writeHandler;}
	void setErrorHandler(callBack&& errorHandler){errorHandler_ = errorHandler;}
	void setConnHandler(callBack &&connHandler) { connHandler_ = connHandler; }
	
	void handleRead();
	void handleWrite();
	void handleConn();

	void handleError(int fd, int err_num, std::string short_msg);
	
	void handleEvents();
	bool isNoneEvent(){return events_ == 0;}
	void disableAll(){events_ = 0;};

	void setEvents(__uint32_t ev){events_ = ev;}
	__uint32_t getEvents(){return events_;}
	__uint32_t getLastEvents() { return lastEvents_; }
	void setRevents(__uint32_t ev){revents_ = ev;}
	
	bool equalAndUpdateLastEvents() 
	{
		bool ret = (lastEvents_ == events_);
		lastEvents_ = events_;
		return ret;
	}
	
	void setHolder(std::shared_ptr<HttpConn> holder) { holder_ = holder; }
	std::shared_ptr<HttpConn> getHolder() 
	{
		std::shared_ptr<HttpConn> ret(holder_.lock());
		return ret;
	}
	

private:

	Eventloop* loop_;
	int fd_;
	__uint32_t events_;
	__uint32_t revents_;
	__uint32_t lastEvents_;
	
private:
	callBack readHandler_;
	callBack writeHandler_;
	callBack errorHandler_;
	callBack connHandler_;
	std::weak_ptr<HttpConn> holder_;
};

#endif