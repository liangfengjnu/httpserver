#include "server.h"

using namespace std;

Server::Server(Eventloop* loop, int port)
:loop_(loop),
 port_(port), 
 acceptChannel_(new Channel(loop_))
{
	
	/*
	try{
		pool_ = new threadpool<httpConn>;
	}
	catch(...){
		return 1;
	}

	epoller_ = new epoller();
	users_ = new httpConn[MAX_FD];
	assert(users);
*/
	if(initServer())
	{
		//isClose_ = true;
	}
}

bool Server::initServer()
{
	listenFd_ = socket(PF_INET, SOCK_STREAM, 0);
	if(listenFd_ < 0) {
        printf("Create socket error!", port_);
        return false;
    }	
	
	int ret = 0;
	struct linger tmp = {1, 0};
	setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    if(ret < 0) {
        close(listenFd_);
        printf("Init linger error!", port_);
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
	address.sin_port = htons(port);
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
	
	//epoll_event events[MAX_EVENT_NUMBER];
	//int epollfd = epoll_create(5);
	
	/*
	ret = epoller_->addFd(listenFd_, listenEvent_ | EPOLLIN);
	
	if(ret == 0) 
	{
        printf("Add listen error!");
        close(listenFd_);
        return false;
    }
	*/
	//httpConn::m_epollfd = epollfd;
}

Server::~Server()
{
	/*
	close(epollfd);
	close(listenfd);
	delete []users;
	delete pool_;
*/
}

void Server::handleNewConn()
{
	struct sockaddr_in client_address;
	socklen_t client_addrlength = sizeof(client_address);
	int connFd = accept(listenFd_, (struct sockaddr*)&client_address, &client_addrlength);
	if(connFd < 0)
	{
		printf("error is: %d\n", errno);
	}
	
	httpConn* conn = new HttpConn(loop_, connFd);
	conn->setHandleMessages(std::bind(&Server::onMessages(), this));
	//conn_->set
}

void Server::start()
{
	acceptChannel_->setFd(listenFd_);
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
	loop_->addToPoller(acceptChannel_);
}

void Server::onMessages()
{
	
}
