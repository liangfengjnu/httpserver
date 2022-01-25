#include "httpconn.h"
#include "channel.h"
#include "eventloop.h"

#include <errno.h>

const int DEFAULT_EXPIRED_TIME = 2000;              // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms

HttpConn::HttpConn(Eventloop* loop, int connFd):
	loop_(loop),
	connFd_(connFd),
	error_(false),
	connectionState_(H_CONNECTED),
	method_(GET),
	version_(HTTP_11),
	state_(STATE_PARSE_URI),
	keepAlive_(false),
	channel_(new Channel(loop_))
{
	printf("httpconn construction : %d\n", connFd_);
	channel_->setFd(connFd_);
	
	//channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	channel_->setReadHandler(std::bind(&HttpConn::handleRead, this));
	channel_->setWriteHandler(std::bind(&HttpConn::handleWrite, this));
	channel_->setConnHandler(bind(&HttpConn::handleConn, this));
}

void HttpConn::handleRead()
{
	printf("read function\n");	
	int saveErrno = 0;  //要改
	int bytes_read = 0;

	bytes_read = readBuff_.readFd(connFd_, &saveErrno);
	if (bytes_read > 0)
	{
		printf("read bytes are : %d\n", bytes_read);
		//handleMessages(readBuff_);
	}
	else if (bytes_read == 0)
	{
		printf("handleClose()\n");
		handleClose();
	}
	else
	{
		printf("error is : %d", saveErrno);
		errno = saveErrno;
		printf("TcpConnection::handleRead\n");
		//handleClose();
		printf("after close\n");
	}

}

void HttpConn::handleClose()
{
	connectionState_ = H_DISCONNECTED;
	channel_->disableAll();
	HttpConnPtr guardThis(shared_from_this());
	loop_->removeChannel(channel_);
}

void HttpConn::handleWrite()
{
	printf("handleWrite()\n");

}

void HttpConn::handleConn()
{
	__uint32_t events_ = channel_->getEvents();
	if (!error_ && connectionState_ == H_CONNECTED) 
	{
		if (events_ != 0) 
		{
			// int timeout = DEFAULT_EXPIRED_TIME;
			// if (keepAlive_) 
				// timeout = DEFAULT_KEEP_ALIVE_TIME;
			if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) 
			{
				events_ = __uint32_t(0);
				events_ |= EPOLLOUT;
			}
			events_ |= EPOLLET;
			loop_->updateToChannel(channel_);
		}
		else if (keepAlive_) 
		{
			events_ |= (EPOLLIN | EPOLLET);
			int timeout = DEFAULT_KEEP_ALIVE_TIME;
			loop_->updateToChannel(channel_);
		}
		else 
		{
			events_ |= (EPOLLIN | EPOLLET);
			//int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
			loop_->updateToChannel(channel_);
		}
	} 
	else if (!error_ && connectionState_ == H_DISCONNECTING &&
             (events_ & EPOLLOUT)) 
	{
		events_ = (EPOLLOUT | EPOLLET);
	} 
	else 
	{
		printf("close with errors\n");
		handleClose();
	}
}

void HttpConn::handleMessages(Buffer& readBuff_)
{
	if(handleMessages_)
		handleMessages_(readBuff_);
}

void HttpConn::newEvent()
{
	channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	loop_->addToPoller(channel_, 0);
}