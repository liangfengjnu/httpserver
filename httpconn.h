#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <unistd.h>
#include <sys/socket.h>


class httpConn{
public:
	httpConn(){}
	~httpConn(){}
	
	//初始化新接受的连接
	void init(int sockfd, const sockaddr_in& addr);
	
	//关闭连接
	void closeConn(bool realClose = true);
	
	//处理客户请求
	void process();
	
	//非阻塞读操作
	bool read();
	
	//非阻塞写操作
	bool write();
	
public:
	//文件名的最大长度
	static const int FILENAME_LEN = 200;
	//读缓冲区的大小
	static const int READ_BUFFER_SIZE = 2048;
	//写缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 1024;

	//HTTP请求方法
	enum METHOD{GET = 0, POST, HEAD, PUT, DELETE,
				TRACE, OPTIONS, CONNECT, PATCH};
	
	//解析客户请求时，主状态机所处的状态
	enum CHECK_STATE{CHECK_STATE_REQUESTLINE = 0,
					 CHECK_STATE_HEADER,	
					 CHECK_STATE_CONTENT};
	
	static int m_epollfd;
	
	//统计用户数量
	static int m_user_count;
	
private:
	void init();
	
	//被process_write调用以填充HTTP应答
	void unmap();
	
private:
	// 该HTTP连接的socket和对方的socket地址
	int m_sockfd;
	sockaddr_in m_address;
	
	//读缓冲区
	char m_read_buf[READ_BUFFER_SIZE];
	//写缓冲区
	char m_write_buf[WRITE_BUFFER_SIZE];
	//当前正在解析的行的起始位置
	int m_start_line;
	//当前正在分析的字符在读缓冲区中的位置
	int m_check_idx;
	//标识读缓冲中已经读入的客户数据的最后一个字节的下一个位置
	int m_read_idx;
	//写缓冲区中待发送的字节数
	int m_write_idx;
	
	
	//主状态机当前所处的状态
	CHECK_STATE m_check_state;
	//请求方法
	METHOD m_method;
	
	//客户请求的目标文件的完整路径
	char m_real_file[FILENAME_LEN];
	//客户请求的目标文件的文件名
	char* m_url;
	//HTTP协议版本号
	char* m_version;
	//HTTP请求的消息体的长度
	int m_content_length;
	//主机名
	char* m_host;
	
	
	
	//HTTP请求是否要求保持连接
	bool m_linger;
	
	//客户请求的目标文件被mmap到内存中的起始位置
	char* m_file_address;
	//目标文件的状态。用以判断文件是否存在、是否为目录、是否可读，并获取文件大小等信息
	struct stat m_file_stat;
	
	//writev的两个成员，其中m_iv_count表示被写内存块的数量
	struct iovec m_iv[2];
	int m_iv_count;
	
}


#endif