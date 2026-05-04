#include "Buffer.h"

#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstddef>

using namespace base;

// 双缓冲区设计：buffer + 栈上extrabuf
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // 栈上预分配64kb缓冲区
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();

    // 第一块缓冲区：buffer的可写区域
    vec[0].iov_base = writerIt();
    vec[0].iov_len = writable;
    // 第二块缓冲区：extrabuf
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 只有当buffer的可写缓冲区不够时，才会使用extrabuf
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) {
        // 出错，保存errno
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        // buffer的可写缓冲区容量充足，更新写索引
        writerIndex_ += n;
    } else {
        // 容量不充足，buffer被写满
        writerIndex_ = buffer_.size();
        // 余下的数据在extrabuf中，紧接着会被添加到buffer尾部，期间可能会触发kuo
        append(extrabuf, n - writable);
    }

    return n;
}

void Buffer::makeSpace(size_t len) {
    // buffer可写入的空间 + 读索引前的预留空间 < 要写入的字节数 + 最小预留字节数（kCheapPrepend：8字节）
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
        size_t readable = readableBytes();
        // 创建一个新的缓冲区，容量为可读数据 + 要写入数据 + 最小预留字节数
        Buffer other(readable + len);
        assert(other.peek() == other.begin() + kCheapPrepend);
        // 将当前可读数据复制到新缓冲区的可写区域中
        std::copy(begin() + readerIndex_, begin() + writerIndex_, other.peek());
        // 交换当前缓冲区和新缓冲区
        swap(other);
        // 更新读写索引
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
        // 确保操作逻辑正确
        assert(kCheapPrepend + readable == writerIndex_);
        assert(readable == readableBytes());
        assert(readerIndex_ == kCheapPrepend);
    } else {
        // 这个情况是，可写入的空间不够，但是如果把前面预留空间多出来的加上就够了
        // 这时可以向前移动可读数据
        // 此时前面预留的字节数一定大于最小预留字节数，否则不需要移动
        assert(kCheapPrepend < readerIndex_);
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
        // 更新读写索引
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;

        // 确保操作逻辑正确
        assert(kCheapPrepend + readable == writerIndex_);
        assert(readable == readableBytes());
    }
}