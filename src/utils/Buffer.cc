#include "Buffer.h"

#include <sys/types.h>
#include <sys/uio.h>

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace utils {
// 双缓冲区设计：buffer + 栈上extrabuf
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // 栈上预分配64kb缓冲区
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();

    // 第一块缓冲区：buffer的可写区域
    vec[0].iov_base = beginWrite();
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

ssize_t Buffer::writeFd(int fd, int* savedErrno) {
    const size_t readable = readableBytes();

    struct iovec vec[1];
    vec[0].iov_base = peek();
    vec[0].iov_len = readable;

    const ssize_t n = ::writev(fd, vec, 1);

    if (n < 0) {
        // 出错，保存errno
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= readable) {
        // 写出的字节小于可读字节数，更新读索引
        readerIndex_ += n;
    } else {
        // 可读字节全部写出，直接重置读写索引
        retrieveAll();
    }

    return n;
}

void Buffer::makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
        // buffer可写入的空间 + 读索引前的预留空间小于要写入的字节数 + 最小预留字节数（kCheapPrepend：8字节）
        // 扩容，增加len字节容量
        buffer_.resize(writerIndex_ + len);
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

        // 确保操作逻辑争取
        assert(kCheapPrepend + readable == writerIndex_);
        assert(readable == readableBytes());
    }
}
}  // namespace utils