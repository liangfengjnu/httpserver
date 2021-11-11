#include "httpconn.h"


const char* ok_200_title = "OK";
const char* error_400_title = "BAD_REQUEST";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the requested file.\n";
const char* doc_root = "/var/www/html";

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

int httpConn::m_user_count = 0;
int httpConn::m_epollfd = -1;


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

//往缓冲中写入待发送的数据
bool httpConn::addResponse(const char* format, ... ){
	if(m_write_idx >= WRITE_BUFFER_SIZE){
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx,
						format, arg_list);
	if(len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)){
		return false;
	}
	m_write_idx += len;
	va_end(arg_list);
	return true;
}

bool httpConn::addStatusLine(int status, const char* title){
	return addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool httpConn::addHeaders(int content_len){
	addContentLength(content_len);
	addLinger();
	addBlankLine();
}

bool httpConn::addContentLength(int content_len){
	return addResponse("Content_Length: %d\r\n", content_len);
}

bool httpConn::addLinger(){
	return addResponse("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool httpConn::addBlankLine(){
	return addResponse("%s", "\r\n");
}

bool httpConn::addContent(const char* content){
	return addResponse("%s", content);
}

//根据服务器处理HTTP请求的结果，决定返回给客户端的内容
bool httpConn::process_write(HTTP_CODE ret){
	switch(ret){
		case INTERNAL_ERROR:
		{
			addStatusLine(500, error_500_title);
			addHeaders(strlen(error_500_form));
			if(!addContent(error_500_form)){
				return false;
			}
			break;
		}
		case BAD_REQUEST:
		{
			addStatusLine(400, error_400_title);
			addHeaders(strlen(error_400_form));
			if(!addContent(error_400_form)){
				return false;
			}
			break;
		}
		case NO_REQUEST:
		{
			addStatusLine(404, error_404_title);
			addHeaders(strlen(error_404_form));
			if(!addContent(error_404_form)){
				return false;
			}
			break;
		}
		case FORBIDDEN_REQUEST:
		{
			addStatusLine(403, error_403_title);
			addHeaders(strlen(error_403_form));
			if(!addContent(error_403_form)){
				return false;
			}
			break;
		}
		case: FILE_REQUEST:
		{
			addStatusLine(200, ok_200_title);
			if(m_file_stat.st_size != 0){
				addHeaders(m_file_stat.st_size);
				m_iv[0].iov_base = m_write_buf;
				m_iv[0].iov_len = m_write_idx;
				m_iv[1].iov_base = m_file_address;
				m_iv[1].iov_len = m_file_stat.st_size;
				m_iv_count = 2;
				return true;
			}
			else{
				const char* ok_string = "<html><body></body></html>";
				addHeaders(strlen(ok_string));
				if(!addContent(ok_string)){
					return false;
				}
			}
		}
		default:
			return false;
	}
	m_iv[0].iov_base = m_write_buf;
	m_iv[0].iov_len = m_write_idx;
	m_iv_count = 1;
	return true;
}

//由线程池中的工作线程调用，这是处理HTTP请求的入口函数
void httpConn::process(){
	HTTP_CODE read_ret = process_read();
	if(read_ret == NO_REQUEST){
		modfd(m_epollfd, m_sockfd, EPOLLIN);
		return;
	}
	bool write_ret = process_write(read_ret);
	if(!write_ret){
		closeConn();
	}
	modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

//从状态机
httpConn::LINE_STATUS httpConn::parse_line(){
	char temp;
	for( ; m_check_idx < m_read_idx; ++m_check_idx){
		temp = m_read_buf[m_check_idx];
		if(temp == '\r'){
			if((m_check_idx + 1) == m_read_idx){
				return LINE_OPEN;
			}
			else if(m_read_buf[m_check_idx + 1] == '\n'){
				m_read_buf[m_check_idx++] = '\0';
				m_read_buf[m_check_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if(temp == '\n'){
			if((m_check_idx > 1) && (m_read_buf[m_check_idx - 1] == '\r')){
				m_read_buf[m_check_idx - 1] = '\0';
				m_read_buf[m_check_idx + 1] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}

//主状态机
httpConn::HTTP_CODE httpConn::process_read(){
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	char* text = 0;
	
	while((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK))
		   || ((line_status = parse_line()) == LINE_OK){
		text = get_line();
		m_start_line = m_check_idx;
		printf("got 1 http line : %s\n", text);
		
		switch(m_check_state){
			case CHECK_STATE_REQUESTLINE:
			{
				ret = parse_request_line(text);
				if(ret == BAD_REQUEST){
					return BAD_REQUEST;
				}
				break;
			}
			case CHECK_STATE_HEADER:
			{
				ret = parse_headers(text);
				if(ret ==  BAD_REQUEST){
					return BAD_REQUEST;
				}
				else if(ret == GET_REQUEST){
					return do_request();
				}
				break;
			}
			case CHECK_STATE_CONTENT:
			{
				ret = parse_content(text);
				if(ret == GET_REQUEST){
					return do_request();
				}
				line_status = LINE_OPEN;
				break;
			}
			default:
				return INTERNAL_ERROR;
		}
	}
	return NO_REQUEST;
}

//解析HTTP请求行，获得请求方法、目标URL，以及HTTP版本号
httpConn::HTTP_CODE httpConn::parse_request_line(char* text){
	m_url = strpbrk(text, " \t");
	if(!m_url){
		return BAD_REQUEST;
	}
	*m_url++ = '\0';
	
	char* method = text;
	if(strcasecmp(method, "GET") == 0){
		m_method = GET;
	}
	else{
		return BAD_REQUEST;
	}
	
	m_url += strspn(m_url, " \t");
	m_version = strpbrk(m_url, " \t");
	if(!m_version){
		return BAD_REQUEST;
	}
	*m_version++ = '\0';
	m_version += strspn(m_version, " \t");
	if(strcasecmp(m_version, "HTTP/1.1" != 0)){
		return BAD_REQUEST;
	}
	if(strncasecmp(m_url, "HTTP://", 7) == 0){
		m_url += 7;
		m_url = strchr(m_url, '/');
	}
	if(!m_url || m_url[0] != '/'){
		return BAD_REQUEST;
	}
	
	m_check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

//解析HTTP请求的一个头部信息
httpConn::HTTP_CODE httpConn::parse_headers(char* text){
	//遇到空行，表示头部字段解析完毕
	if(text[0] == '\0'){
		/*
		如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体，
		状态机转移到CHECK_STATE_CONTENT状态
		*/
		if(m_content_length != 0){
			m_check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		
		//否则说明我们已经得到了一个完整的HTTP请求
		return GET_REQUEST;
	}
	//处理Connection头部字段
	else if(strncasecmp(text, "Connection:", 11) == 0){
		text += 11;
		text += strspn(text, " \t");
		if(strcasecmp(text, "keep-alive") == 0){
			m_linger = true;
		}
	}
	
	//处理Content-Length头部字段
	else if(strncasecmp(text, "Content-Length:", 15) == 0){
		text += 15;
		text += strspn(text, " \t");
		m_content_length = atol(text);
	}
	
	//处理Host头部字段
	else if(strncasecmp(text, "Host:", 5) == 0){
		text += 5;
		text += strspn(text, " \t");
		m_host = text;
	}
	else{
		printf("oop! unknow header %s\n", text);
	}
	return NO_REQUEST;
}

httpConn::HTTP_CODE httpConn::parse_content(char* text){
	if(m_read_idx >= (m_content_length + m_check_idx)){
		text(m_content_length] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

//分析目标文件的属性
httpConn::HTTP_CODE httpConn::do_request(){
	strcpy(m_real_file, doc_root);
	int len = strlen(dco_root);
	strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
	if(stat(m_real_file, &m_file_stat) < 0){
		return NO_RESOURCE;
	}
	if(!(m_file_stat.st_mode & S_IROTH)){
		return FORBIDDEN_REQUEST;
	}
	if(S_ISDIR(m_file_stat.st_mode)){
		return BAD_REQUEST;
	}
	int fd = open(m_real_file, O_RDONLY);
	m_file_address = (char*)mmap(0, m_file_stat.st_size, PORT_READ, 
								 MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
}