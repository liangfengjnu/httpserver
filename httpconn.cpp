#include "httpconn.h"
#include "channel.h"
#include "eventloop.h"
#include "timer.h"

#include <errno.h>

const int DEFAULT_EXPIRED_TIME = 2000;              // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms

const char* HttpConn::srcDir;

HttpConn::HttpConn(Eventloop* loop, int connFd):
	loop_(loop),
	connFd_(connFd),
	isClose_(false),
	connectionState_(H_CONNECTED),
	method_(GET),
	version_(HTTP_11),
	state_(STATE_PARSE_URI),
	keepAlive_(false),
	channel_(new Channel(loop_))
{
	printf("httpconn construction : %d\n", connFd_);
	channel_->setFd(connFd_);
	writeBuff_.retrieveAll();
    readBuff_.retrieveAll();
	srcDir = getcwd(nullptr, 256);
	//channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	channel_->setReadHandler(std::bind(&HttpConn::handleRead, this));
	channel_->setWriteHandler(std::bind(&HttpConn::handleWrite, this));
}

void HttpConn::handleRead()
{
	ssize_t len = -1;
	int readErrno;
	len = readBuff_.readFd(connFd_, &readErrno);
	printf("read bytes are : %d\n", len);
	if(len <= 0 && readErrno != EAGAIN)
	{
		printf("read error\n");
		//error_ = true;
		handleClose();
		return ;
	}
	toProcess();
}


void HttpConn::toProcess()
{
	if(process())
	{
		channel_->setEvents(EPOLLOUT | EPOLLET | EPOLLONESHOT);
		handleWrite();
	}
	else
	{
		channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	}
	loop_->updateToChannel(channel_);
	
}

bool HttpConn::process()
{
    request_.init();
    if(readBuff_.readableBytes() <= 0) 
	{
        return false;
    }
    else if(request_.parse(readBuff_)) 
	{
        //LOG_DEBUG("%s", request_.path().c_str());
		printf("%s\n", request_.path().c_str());
        response_.init(srcDir, request_.path(), request_.isKeepAlive(), 200);
    } 
	else 
	{
        response_.init(srcDir, request_.path(), false, 400);
    }

    response_.makeResponse(writeBuff_);
    /* 响应头 */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.peek());
    iov_[0].iov_len = writeBuff_.readableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if(response_.fileLen() > 0  && response_.file()) 
	{
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    //LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}

void HttpConn::handleClose()
{
	connectionState_ = H_DISCONNECTED;
	channel_->disableAll();
	//HttpConnPtr guardThis(shared_from_this());
	loop_->removeChannel(channel_);
	response_.unmapFile();
	if(isClose_ == false)
	{
        isClose_ = true; 
        //userCount--;
        close(connFd_);
        //LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

void HttpConn::handleWrite()
{
	printf("handleWrite\n");
    int ret = -1;
    int writeErrno = 0;
    ret = onWrite(&writeErrno);
    if(toWriteBytes() == 0) 
	{
        /* 传输完成 */
        if(isKeepAlive()) 
		{
            toProcess();
            return;
        }
    }
    else if(ret < 0) 
	{
        if(writeErrno == EAGAIN) 
		{
            /* 继续传输 */
            channel_->setEvents(EPOLLOUT | EPOLLET | EPOLLONESHOT);
			loop_->updateToChannel(channel_);
            return;
        }
    }
    handleClose();
}

ssize_t HttpConn::onWrite(int *saveErrno)
{
	ssize_t len = -1;
    do 
	{
        len = writev(connFd_, iov_, iovCnt_);
        if(len <= 0) 
		{
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov_[0].iov_len) 
		{
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) 
			{
                writeBuff_.retrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else 
		{
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.retrieve(len);
        }
    } while(toWriteBytes() > 10240);
    return len;
}

void HttpConn::newEvent()
{
	channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	loop_->addToPoller(channel_, 0);
}

void HttpConn::linkTimer(std::shared_ptr<TimerNode> mtimer) 
{
	// shared_ptr重载了bool, 但weak_ptr没有
	timer_ = mtimer;
}