#include "server.h"
#include "eventloop.h"


using namespace std;
using std::placeholders::_1;


void handle_for_sigpipe() 
{
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	if(sigaction(SIGPIPE, &sa, NULL)) return;
}


int setSocketNonBlocking(int fd) 
{
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1) return -1;

	flag |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flag) == -1) return -1;
	return 0;
}

void setSocketNodelay(int fd) 
{
	int enable = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}

int socket_bind_listen(int port) 
{
  // 检查port值，取正确区间范围
  if (port < 0 || port > 65535) return -1;

  // 创建socket(IPv4 + TCP)，返回监听描述符
  int listen_fd = 0;
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

  // 消除bind时"Address already in use"错误
  int optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    close(listen_fd);
    return -1;
  }

  // 设置服务器IP和Port，和监听描述副绑定
  struct sockaddr_in server_addr;
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    close(listen_fd);
    return -1;
  }

  // 开始监听，最大等待队列长为LISTENQ
  if (listen(listen_fd, 2048) == -1) {
    close(listen_fd);
    return -1;
  }

  // 无效监听描述符
  if (listen_fd == -1) {
    close(listen_fd);
    return -1;
  }
  return listen_fd;
}

Server::Server(Eventloop* loop, int port):
	//threadNum_(threadNum),
	loop_(loop),
	port_(port), 
	nextConnId_(0),
	acceptChannel_(new Channel(loop_))
	//listenFd_(socket_bind_listen(port_))
	//eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),
{
	handle_for_sigpipe();
	
	// if(setSocketNonBlocking(listenFd_) < 0) 
	// {
		// perror("set socket non block failed");
		// abort();
	// }
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
	// for (auto& item : ConnectionMap_)
	// {
		// HttpConnPtr conn(item.second);
		// item.second.reset();
		// conn->connectDestroy();
	// }
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
	memset(&client_address, 0, sizeof(struct sockaddr_in));
	socklen_t client_addrlength = sizeof(client_address);
	int connFd = 0;
//	printf("handleNewConn function\n");
	while((connFd = accept(listenFd_, (struct sockaddr*)&client_address,
					&client_addrlength)) > 0)
	{
		if(setSocketNonBlocking(connFd) < 0)
		{
			printf("Set non block failed!\n");
			return;
		}
			printf("connection %d is succuess\n", connFd);
			
		// char buf[64];
		// snprintf(buf, sizeof buf, "-%d#%d", port_, nextConnId_);
		// ++nextConnId_;
		// string connName = buf;

		setSocketNodelay(connFd);
		HttpConnPtr conn (new HttpConn(loop_, connFd));
		//ConnectionMap_[connName] = conn;
		conn->setHandleMessages(std::bind(&Server::onRequest, this, _1));
		conn->setCloseCallBack(std::bind(&Server::removeConnection, this, _1));
		conn->newEvent();
	}
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}

void Server::start()
{
	acceptChannel_->setFd(listenFd_);
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
	acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));
	loop_->addToPoller(acceptChannel_.get());
}

void Server::handThisConn()
{ 
	loop_->updateToChannel(acceptChannel_.get()); 
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