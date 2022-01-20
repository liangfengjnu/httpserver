#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
	
	void start();
	void handleNewConn();
	void onRequest(Buffer& buffer);
	
private:
	bool initServer();
	
private:
	Eventloop* loop_;
	HttpRequest request_;
	int listenFd_;
	int port_;
	std::shared_ptr<Channel> acceptChannel_;
	/*
	static const int MAX_FD = 65536;
	static const int MAX_EVENT_NUMBER = 10000;
	int listenFd_;
	bool isClose_;
	int userCount_;
	httpConn* users_;
	
	
	std::unique_ptr<epoller> epoller_;
	std::unique_ptr<httpConn> pool_;
	*/
};

#endif