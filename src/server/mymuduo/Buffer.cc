#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据，放入Buffer的writable区域  Poller工作在LT模式  
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 * （提供给TcpConnection::handleRead中的inputBuffer_使用）
 */ 
ssize_t Buffer::readFd(int fd, int* saveErrno) 
{
    char extrabuf[65536] = {0}; // 栈上的内存空间  64K
    
    struct iovec vec[2];
    
    const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_; // 可写缓冲区的起始位置
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf; 
    vec[1].iov_len = sizeof extrabuf;
    
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt); // 从fd上读取数据，放入Buffer的writable区域，iovcnt=2时还需把数据放入extrabuf中
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_ += n; // writerIndex_向后移动n个位置
    }
    else // extrabuf里面也写入了数据 
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);  // writerIndex_开始写 n - writable大小的数据
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno) // 将Buffer中的可读数据全部写给fd文件（TcpConnection::handleWrite中的outputBuffer_使用）
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}