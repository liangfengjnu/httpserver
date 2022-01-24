#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <memory>

#include "httpconn.h"
#include "httprequest.h"
#include "channel.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

class Eventloop;

class Server
{
public:
	Server(Eventloop* loop, int port);
	
	~Server();
	
	Eventloop *getLoop() const { return loop_; }
	void start();
	void handleNewConn();
	void onRequest(Buffer& buffer);
	void removeConnection(const HttpConnPtr& conn);
	void handThisConn();
	
private:
	bool initServer();
	
private:
	Eventloop* loop_;
	//int threadNum_;
	HttpRequest request_;
	int listenFd_;
	int port_;
	int nextConnId_;
	std::shared_ptr<Channel> acceptChannel_;
	std::map<std::string, HttpConnPtr> ConnectionMap_;
	//std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
};

#endif