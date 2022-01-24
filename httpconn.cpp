#include "httpconn.h"
#include "channel.h"
#include "eventloop.h"

#include <errno.h>

const char* ok_200_title = "OK";
const char* error_400_title = "BAD_REQUEST";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";
const char* doc_root = "/var/www/html";


HttpConn::HttpConn(Eventloop* loop, int connFd):
	loop_(loop),
	connFd_(connFd),
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
		handleMessages(readBuff_);
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

	channel_->disableAll();
	HttpConnPtr guardThis(shared_from_this());
	closeCallBack_(guardThis);
}

void HttpConn::handleWrite()
{
	printf("handleWrite()\n");
	
   // ssize_t len = -1;
   // int saveErrno = 0; 
    // do 
	// {
        // len = writev(fd_, iov_, iovCnt_);
        // if(len <= 0) {
            // saveErrno = errno;
            // break;
        // }
        // if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        // else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            // iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            // iov_[1].iov_len -= (len - iov_[0].iov_len);
            // if(iov_[0].iov_len) {
                // writeBuff_.retrieveAll();
                // iov_[0].iov_len = 0;
            // }
        // }
        // else {
            // iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            // iov_[0].iov_len -= len; 
            // writeBuff_.retrieve(len);
        // }
    // } while(EPOLLET || iov_[0].iov_len + iov_[1].iov_len > 10240);
    
	// if(iov_[0].iov_len + iov_[1].iov_len == 0)
	// {
		//写完了
	// }else if(len < 0)
	// {
		// if(saveErrno == EAGAIN)
		// {
			//继续传输
			// channel_->setEvents(EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
			// channel_->update();
			// return ;
		// }
	// }
}

void HttpConn::handleConn()
{
	
}

void HttpConn::handleMessages(Buffer& readBuff_)
{
	if(handleMessages_)
		handleMessages_(readBuff_);
}

void HttpConn::connectDestroy()
{
	channel_->disableAll();
	channel_->remove();
}

void HttpConn::newEvent()
{
	channel_->setEvent(EPOLLIN | EPOLLET | EPOLLONESHOT);
	loop_->addToPoller(channel_.get());
}