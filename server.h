#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>

#include "httpconn.h"
#include "channel.h"
#include "eventloop.h"
#include "eventloopthreadpool.h"


class Eventloop;

class Server
{
public:
	Server(Eventloop* loop, int port);
	
	~Server();
	
	Eventloop *getLoop() const { return loop_; }
	void start();
	void handleNewConn();
	
private:
	bool initServer();
	
private:
	Eventloop* loop_;
	int threadNum_;
	int listenFd_;
	int port_;
	int nextConnId_;
	bool started_;
	std::shared_ptr<Channel> acceptChannel_;
//	std::map<std::string, HttpConnPtr> ConnectionMap_;
	std::unique_ptr<EventloopThreadPool> eventLoopThreadPool_;
};

#endif