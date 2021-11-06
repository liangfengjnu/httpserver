#include "httpconn.h"

void setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETEL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, newoption);
	return old_option;
}

void addfd(int epollfd, int fd, bool one_shot){
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	if(one_shot){
		event.events |= EPOLLONESHOT;
	}
	epoll_clt(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

void removefd(int epollfd, int fd){
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

void modfd(int epollfd, int fd, int ev){
	epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	epoll_clt(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void httpConn::closeConn(bool realClose){
	if(realClose && (m_sockfd == -1)){
		removefd(m_epollfd, m_sockfd);
		m_sockfd = -1;
		m_user_count--;
	}
}

void httpConn::init(int sockfd, const sockaddr_in& addr){
	m_sockfd = sockfd;
	m_address = addr;
	
	// 调试，为了避免TIME_WAIT状态
	int reuse = 1;
	setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	addfd(m_epollfd, sockfd, true);
	m_user_count++;
	
	init();
}

void httpConn::init(){
	m_check_state = CHECK_STATE_REQUESTLINE;
	m_linger = false;
	m_method = GET;
	m_url = 0;
	m_version = 0;
	m_content_length = 0;
	m_host = 0;
	m_start_line = 0;
	m_check_idx = 0;
	m_read_idx = 0;
	m_write_idx = 0;
	
	memset(m_read_buf, '\0', READ_BUFFER_SIZE);
	memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
	memset(m_real_file, '\0', FILENAME_LEN);
}

bool httpConn::read(){
	if(m_read_idx >= READ_BUFFER_SIZE){
		return false;
	}
	
	int bytes_read = 0;
	while(true){
		bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, 
						  READ_BUFFER_SIZE - m_read_idx, 0);
		if(bytes_read == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				break;
			}
			return false;
		}
		else if(bytes_read == 0)
		{
			return false;
		}
		m_read_idx += bytes_read;
	}
	return true;
}

//对内存映射区执行munmap操作
void httpConn::unmap(){
	if(m_file_address){
		munmap(m_file_address, m_file_stat.st_size);
		m_file_address = 0;
	}
}

//写HTTP响应
bool httpConn::write(){
	int temp = 0;
	int bytes_have_send = 0;
	int bytes_to_send = m_write_idx;
	if(bytes_to_send == 0){
		modfd(m_epollfd, m_sockfd, EPOLLIN);
		init();
		return true;
	}
	
	while(true){
		temp = writev(m_sockfd, m_iv, m_iv_count);
		if(temp == -1){
			if(errno == EAGAIN){
				modfd(m_epollfd, m_sockfd, EPOLLOUT);
				return true;
			}
			unmap();
			return false;
		}
		bytes_to_send -= temp;
		bytes_have_send += temp;
		if(bytes_to_send <= bytes_have_send){
			unmap();
			if(m_linger){
				init();
				modfd(m_epollfd, m_sockfd, EPOLLIN);
				return true;
			}
			else{
				modfd(m_epollfd, m_sockfd, EPOLLIN);
				return false;
			}
		}
	}
}