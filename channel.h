#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>
#include <sys/epoll.h>
#include <memory>


class Eventloop;

class Channel
{
private:
	typedef std::function<void()> callBack;
public:
	Channel(Eventloop* loop);
	~Channel();
	
	void setFd(int fd){fd_ = fd;}
	int getFd(){return fd_;}
	
	void setReadHandler(callBack&& readHandler){readHandler_ = readHandler;}
	void setWriteHandler(callBack&& writeHandler){writeHandler_ = writeHandler;}
	void setErrorHandler(callBack&& errorHandler){errorHandler_ = errorHandler;}
	void setConnHandler(callBack&& connHandler){connHandler_ = connHandler;}
	
	void handleRead();
	void handleWrite();
	void handleConn();
	
	void handleEvents();
	bool isNoneEvent(){return events_ == 0;}
	void disableAll(){events_ = 0; update();};
	void update();
	void remove();
	
	void setEvents(__uint32_t ev){events_ = ev;}
	__uint32_t getEvents(){return events_;}
	void setRevents(__uint32_t ev){revents_ = ev;}

private:

	Eventloop* loop_;
	int fd_;
	__uint32_t events_;
	__uint32_t revents_;
	
private:
	callBack readHandler_;
	callBack writeHandler_;
	callBack errorHandler_;
	callBack connHandler_;
};

#endif