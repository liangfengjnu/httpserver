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

#include "locker.h"
#include "server.h"
#include "eventloop.h"


void addsig(int sig, void(handler)(int), bool restart = true){
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if(restart){
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

void show_error(int connfd, const char* info){
	printf("%s", info);
	send(connfd, info, strlen(info),  0);
	close(connfd);
}

int main(int argc, char* argv[]){
	
	if(argc < 2){
		printf("usage : %s port_number\n", basename(argv[0]));
		return 1;
	}
	Eventloop mainloop;
	//const char* ip = argv[1];
	int port = atoi(argv[1]);
	addsig(SIGPIPE, SIG_IGN);
	Server server(mainloop, port);
	server.start();
	
	mainloop.loop();
	return 0;
}
