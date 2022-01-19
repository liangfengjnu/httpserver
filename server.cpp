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
		isClose_ = true;
	}
	acceptChannel_->setFd(listenFd_);
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
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

	/*
	ret = listen(listenFd_, 5);
	if(ret < 0) 
	{
        printf("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
	
	//epoll_event events[MAX_EVENT_NUMBER];
	//int epollfd = epoll_create(5);
	
	
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
	int connFd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
	if(connFd < 0)
	{
		printf("error is: %d\n", errno);
		continue;
	}
	
	httpConn* conn = new HttpConn(loop_, connFd);
	//conn_->set
}

void Server::start()
{
/*
	while(！isClose_)
	{
		int number = epoller_->wait(epollfd, events, MAX_EVENT_NUMBER, -1);
		if((number < 0) && (errno != EINTR)){
			printf("epoll failure\n");
			break;
		}
		
		for(int i = 0; i < number; ++i){
			int sockfd = events[i].data.fd;
			if(sockfd == listenfd){
				struct sockaddr_in client_address;
				socklen_t client_addrlength = sizeof(client_address);
				int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
				if(connfd < 0){
					printf("error is: %d\n", errno);
					continue;
				}
				if(httpConn::m_user_count >= MAX_FD){
					show_error(connfd, "Internal server busy");
					continue;
				}
				
				users[connfd].init(connfd, client_address);
			}
			else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
				users[sockfd].closeConn();
			}
			else if(events[i].events & EPOLLIN){
				if(users[sockfd].read()){
					pool_->append(users + sockfd);
				}
				else{
					users[sockfd].closeConn();
				}
			}
			else if(events[i].events & EPOLLOUT){
				if(!users[sockfd].write()){
					users[sockfd].closeConn();
				}
			}
			else {}
		}
	}
*/
}