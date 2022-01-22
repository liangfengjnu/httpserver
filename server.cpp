#include "server.h"
#include "eventloop.h"


using namespace std;
using std::placeholders::_1;



Server::Server(Eventloop* loop, int port):
	//threadNum_(threadNum),
	loop_(loop),
	port_(port), 
	nextConnId_(0),
	acceptChannel_(new Channel(loop_))
	//eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),
{
	if(!initServer())
	{
		printf("server init failed\n");
	}
	
}

Server::~Server()
{
	/*
	close(epollfd);
	close(listenfd);
	delete []users;
	delete pool_;
*/
	for (auto& item : ConnectionMap_)
	{
		HttpConnPtr conn(item.second);
		item.second.reset();
		conn->connectDestroy();
	}
}

bool Server::initServer()
{
	listenFd_ = socket(PF_INET, SOCK_STREAM, 0);
	if(listenFd_ < 0) {
        printf("Create socket error! %d", port_);
        return false;
    }	
	
	int ret = 0;
	struct linger tmp = {1, 0};
	setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    if(ret < 0) {
        close(listenFd_);
        printf("Init linger error! %d", port_);
        return false;
    }

	int optval = 1;

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) 
	{
        printf("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	//inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port_);
	address.sin_addr.s_addr = htonl(INADDR_ANY);	

	ret = bind(listenFd_, (struct sockaddr*)&address, sizeof(address));
	if(ret < 0)
	{
        printf("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

	
	ret = listen(listenFd_, 5);
	if(ret < 0) 
	{
        printf("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

	return true;
}





void Server::handleNewConn()
{
	struct sockaddr_in client_address;
	socklen_t client_addrlength = sizeof(client_address);
	int connFd = accept(listenFd_, (struct sockaddr*)&client_address, &client_addrlength);
	if(connFd <= 0)
	{
		return;
	}
	
	char buf[64];
	snprintf(buf, sizeof buf, "-%d#%d", port_, nextConnId_);
	++nextConnId_;
	string connName = buf;
	
	
	printf("connection %d is succuess\n", connFd);
	//HttpConn* conn = new HttpConn(loop_, connFd);
	HttpConnPtr conn (new HttpConn(loop_, connFd));
	ConnectionMap_[connName] = conn;
	conn->setHandleMessages(std::bind(&Server::onRequest, this, _1));
	conn->setCloseCallBack(std::bind(&Server::removeConnection, this, _1));
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	//conn_->set
}

void Server::start()
{
	acceptChannel_->setFd(listenFd_);
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
	loop_->addToPoller(acceptChannel_.get());
}

//处理请求头
void Server::onRequest(Buffer& readBuff_)
{
	std::cout<<readBuff_.retrieveAllToStr()<<std::endl;

	if(!request_.parse(readBuff_))
	{
		printf("request error!\n");
	}
	else
	{
		//写回复
		//查文件合法
		//写文件
	}
}

void Server::removeConnection(const HttpConnPtr& conn)
{
	conn->connectDestroy();
}