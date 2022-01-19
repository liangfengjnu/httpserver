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

#include "channel.h"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

class Server
{
public:
	Server(Eventloop* loop, int port);
	
	~Server();
	
	void start();
	void handleNewConn();
	
private:
	bool initServer();
	
private:
	Eventloop loop_;
	int listenFd_;
	Channel acceptChannel_;
	/*
	static const int MAX_FD = 65536;
	static const int MAX_EVENT_NUMBER = 10000;
	int port_;
	int listenFd_;
	bool isClose_;
	int userCount_;
	httpConn* users_;
	
	
	std::unique_ptr<epoller> epoller_;
	std::unique_ptr<httpConn> pool_;
	*/
};

#endif