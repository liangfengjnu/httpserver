#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/uio.h>

#include "buffer.h"
#include "callbacks.h"
#include "httprequest.h"
#include "httpresponse.h"

class Channel;
class Eventloop;


enum ProcessState 
{
	STATE_PARSE_URI = 1,
	STATE_PARSE_HEADERS,
	STATE_RECV_BODY,
	STATE_ANALYSIS,
	STATE_FINISH
};

enum ConnectionState{H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED};

enum Method{GET = 0, POST, HEAD, PUT, DELETE,
			TRACE, OPTIONS, CONNECT, PATCH};
			
enum Version{HTTP_10 = 1, HTTP_11};


class HttpConn : public std::enable_shared_from_this<HttpConn>
{
private:
	typedef std::function<void()> callBack;
	typedef std::function<void(Buffer& buffer)> messageCallBack;
	typedef std::function<void (const HttpConnPtr&)> closeCallBack;
public:
	HttpConn(Eventloop* loop, int connFd);
	~HttpConn(){}

	
	//void setCloseCallBack(closeCallBack&& handleClose){closeCallBack_ = handleClose;}
	
	void handleRead();
	void handleWrite();
	void handleClose();
	void handleConn();
	void toProcess();
	bool process();
	ssize_t onWrite(int* saveErrno);
	
	void newEvent();
	
	int toWriteBytes()
	{
		return iov_[0].iov_len + iov_[1].iov_len;
	}
	
    bool isKeepAlive() const 
	{
        return request_.isKeepAlive();
    }

public:
	static const char* srcDir;
	

private:

	
	//读缓冲区
	Buffer readBuff_;
	//写缓冲区
	Buffer writeBuff_;

	
	int connFd_;
	Eventloop* loop_;
	bool isClose_;
	ConnectionState connectionState_;
	Method method_;
	Version version_;
	ProcessState state_;
	bool keepAlive_;



	std::shared_ptr<Channel> channel_; 
	messageCallBack handleMessages_;
	closeCallBack closeCallBack_;
	
	int iovCnt_;
    struct iovec iov_[2];
    HttpRequest request_;
    HttpResponse response_;

};
typedef std::shared_ptr<HttpConn> HttpConnPtr;

#endif
