#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <algorithm>
#include <assert.h>
#include <sys/uio.h>
#include <unistd.h>
#include <iostream>
#include <string>

class Buffer
{
public:
	explicit Buffer(int initBufferSize = 1024)
	:buffer_(1024), readIndex_(0), writeIndex_(0){}
	
	~Buffer() = default;
	
	size_t writableBytes() const
	{
		return buffer_.size() - writeIndex_;
	}
	
    size_t readableBytes() const
	{
		return writeIndex_ - readIndex_;
	}
	
    size_t prependableBytes() const
	{
		return readIndex_;
	}

    const char* peek() const
	{
		return begin() + readIndex_;
	}
	
    void ensureWriteable(size_t len)
	{
		if(writableBytes() < len)
		{
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}
	
    void hasWritten(size_t len)
	{
		assert(len <= writableBytes());
		writeIndex_ += len;
	}

    void retrieve(size_t len)
	{
		assert(len <= readableBytes());
		readIndex_ += len;
	}
	
    void retrieveUntil(const char* end)
	{
		assert(peek() <= end);
		retrieve(end - peek());
	}

    void retrieveAll()
	{
		bzero(&buffer_[0], buffer_.size());
		readIndex_ = 0;
		writeIndex_ = 0;
	}
	
    std::string retrieveAllToStr()
	{
		std::string str(peek(), readableBytes());
		retrieveAll();
		return str;
	}

    const char* beginWriteConst() const
	{
		return begin() + writeIndex_;
	}
	
    char* beginWrite()
	{
		return begin() + writeIndex_;
	}

    void append(const std::string& str)
	{
		append(str.data(), str.length());
	}
	
    void append(const void* data, size_t len)
	{
		assert(data);
		append(static_cast<const char*> (data), len);
	}
	
    void append(const char* data, size_t len)
	{
		assert(data);
		ensureWriteable(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}
	
    void append(const Buffer& buff)
	{
		append(buff.peek(), buff.readableBytes());
	}

    ssize_t readFd(int fd, int* Errno)
	{
		char buff[65526];
		struct iovec iov[2];
		const size_t writable = writableBytes();
		
		//分散读
		iov[0].iov_base = begin() + writeIndex_;
		iov[0].iov_len = writable;
		iov[1].iov_base = buff;
		iov[1].iov_len = sizeof(buff);
		
		const ssize_t len = readv(fd, iov, 2);
		if(len < 0)
		{
			*Errno = errno;
		}
		else if(static_cast<size_t>(len) <= writable) 
		{
			writeIndex_ += len;
		}
		else 
		{
			writeIndex_ = buffer_.size();
			append(buff, len - writable);
		}
		
		return len;
	}
	
    ssize_t writeFd(int fd, int* Errno)
	{
		size_t readSize = readableBytes();
		ssize_t len = write(fd, peek(), readSize);
		if(len < 0) 
		{
			*Errno = errno;
			return len;
		} 
		readIndex_ += len;
		return len;
	}
	
private:
	char* begin()
	{
		return &*buffer_.begin();
	}

	const char* begin() const
	{
		return &*buffer_.begin();
	}
	
	void makeSpace(size_t len)
	{
		if(writableBytes() + prependableBytes() < len)
		{
			buffer_.resize(writeIndex_ + len + 1);
		}else{
			size_t readable = readableBytes();
			std::copy(begin() + readIndex_, begin() + writeIndex_, begin());
			readIndex_ = 0;
			writeIndex_ = readIndex_ + readable;
			assert(readable == readableBytes());
		}
	}
	
	
private:
	std::vector<char> buffer_;
	size_t readIndex_;
	size_t writeIndex_;
	
	
};

#endif
