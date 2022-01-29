#include "server.h"
#include "eventloop.h"
#include "Util.h"


Server::Server(Eventloop* loop, int port):
	threadNum_(2),
	loop_(loop),
	port_(port), 
	nextConnId_(0),
	acceptChannel_(new Channel(loop_)),
	eventLoopThreadPool_(new EventloopThreadPool(loop_, 2))
{
	handle_for_sigpipe();

	if(!initServer())
	{
		printf("server init failed\n");
	}	
	if(setSocketNonBlocking(listenFd_) < 0) 
	{
		perror("set socket non block failed");
		abort();
	}	
	
}

Server::~Server()
{

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

void Server::start()
{
	eventLoopThreadPool_->start();
	acceptChannel_->setFd(listenFd_);
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
	loop_->addToPoller(acceptChannel_, 0);
	started_ = true;
}


void Server::handleNewConn()
{
	struct sockaddr_in client_address;
	memset(&client_address, 0, sizeof(struct sockaddr_in));
	socklen_t client_addrlength = sizeof(client_address);
	int connFd = 0;
	printf("handleNewConn function\n");
	if((connFd = accept(listenFd_, (struct sockaddr*)&client_address,
					&client_addrlength)) > 0)
	{
		Eventloop* loop = eventLoopThreadPool_->getNextLoop();
		if(setSocketNonBlocking(connFd) < 0)
		{
			printf("Set non block failed!\n");
			return;
		}
		printf("connection %d is succuess\n", connFd);

		setSocketNodelay(connFd);
		HttpConnPtr conn (new HttpConn(loop, connFd));
		//conn->newEvent();
		loop->queueInLoop(std::bind(&HttpConn::newEvent, conn));
	}
	//acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	loop_->updateToChannel(acceptChannel_); 
}


