#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>

class Eventloop;

class Channel
{
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
	
	void setEvents(__uint32_t ev){events_ = ev;}
	__uint32_t getEvents(){return events_;}
	void setRevents(__uint32_t ev){revents_ = ev;}

private:
	typedf std::function<void()> callBack;
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