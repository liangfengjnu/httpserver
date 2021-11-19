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

int setnonblocking(int fd){
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

void addfd(int epollfd, int fd, bool one_shot){
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	if(one_shot){
		event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
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
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
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
//	printf("read function\n");	
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
	printf("m_write_idx is %d\n", m_write_idx);
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
			printf("temp == -1\n");
			return false;
		}
		bytes_to_send -= temp;
		bytes_have_send += temp;
		printf("bytes_to_send is %d, bytes_have_send is %d, temp is %d\n",
			       	bytes_to_send, bytes_have_send, temp);
		
		if(bytes_to_send <= bytes_have_send){
			printf("bytes_to_send is %d\n", bytes_to_send);
			printf("bytes_have_send is %d\n", bytes_have_send);
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
			printf("ret is BAD_REQUEST");
			addStatusLine(400, error_400_title);
			addHeaders(strlen(error_400_form));
			if(!addContent(error_400_form)){
				return false;
			}
			break;
		}
		case NO_REQUEST:
		{
			printf("ret is NO_REQUEST");
			addStatusLine(400, error_400_title);
			addStatusLine(404, error_404_title);
			addHeaders(strlen(error_404_form));
			if(!addContent(error_404_form)){
				return false;
			}
			break;
		}
		case FORBIDDEN_REQUEST:
		{
			addStatusLine(400, error_400_title);
			addStatusLine(403, error_403_title);
			addHeaders(strlen(error_403_form));
			if(!addContent(error_403_form)){
				return false;
			}
			break;
		}
		case FILE_REQUEST:
		{
			printf("ret is FILE_REQUEST\n");
			m_linger = true;
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
	HTTP_CODE read_ret = do_read();
	if(read_ret == NO_REQUEST){
		modfd(m_epollfd, m_sockfd, EPOLLIN);
		printf("NO_REQUEST\n");
		return;
	}
	bool write_ret = process_write(read_ret);
	if(!write_ret){
		printf("closeConn function\n");
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
			if(m_check_idx > 1){
				m_read_buf[m_check_idx] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}

httpConn::HTTP_CODE httpConn::http_request(){
	struct stat sbuf;

	int ret = stat(m_url, &m_file_stat);
	if(ret != 0){
		printf("文件错误\n");
		return BAD_REQUEST;
	}
	printf("这里发送文件\n");

	int fd = open(m_url, O_RDONLY);
	m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ,
				     MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
}

httpConn::HTTP_CODE httpConn::do_read(){
	LINE_STATUS line_status = LINE_OK;
	line_status = parse_line();
	char* text = getline();
	printf("got 1 http line : %s\n", text);
	HTTP_CODE ret = parse_request_line(text);
	if(ret == BAD_REQUEST){
		modfd(m_epollfd, m_sockfd, EPOLLIN);
		printf("bad request\n");
		return BAD_REQUEST;
	}

	ret = http_request();
	return ret;
}

//解析HTTP请求行，获得请求方法、目标URL，以及HTTP版本号
httpConn::HTTP_CODE httpConn::parse_request_line(char* text){
	printf("text is %s\n", text);
	char *method = strtok(text, " ");
	m_url = strtok(NULL, " ");
	m_version = strtok(NULL, " ");


	if(strcasecmp(method, "GET") == 0){
		m_method = GET;
	}

	if(strcasecmp(m_version, "HTTP/1.1") != 0){
		return BAD_REQUEST;
	}
	printf("method = %s, path = %s, protocal = %s\n", method, m_url, m_version);


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
		printf("空行标记！\n");	
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
		text[m_content_length] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

//分析目标文件的属性
httpConn::HTTP_CODE httpConn::do_request(){
/*	strcpy(m_real_file, doc_root);
	int len = strlen(doc_root);
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
	m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, 
								 MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
*/
//	struct stat sbuf;
//	int ret = stat(m_url, &sbuf);
//	if(ret != 0){
//		printf("文件不存在");
//		exit(1);
//	}
	return FILE_REQUEST;
}
