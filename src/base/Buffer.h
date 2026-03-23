#pragma once

#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace base {
class Buffer {
   public:
    // 预留8字节，方便添加协议头部
    static constexpr size_t kCheapPrepend = 8;
    // 默认的缓冲区初始大小1kb
    static constexpr size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {
    }

    // 获取可读字节数，写索引位置减去读索引位置
    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    // 获取可写字节数，buffer容量减去写索引位置
    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    // 返回预留的字节数
    //* 这里直接返回读索引位置，读索引一定大于等于预留字节数，这样协议头直接添加，不用专门移动数据
    size_t prependableBytes() const {
        return readerIndex_;
    }

    // 获取读索引所在的指针
    const char* peek() const {
        return begin() + readerIndex_;
    }
    // 获取读索引所在的指针
    char* peek() {
        return begin() + readerIndex_;
    }

    // 检索len个可读字符，读索引向后移动len
    void retrieve(size_t len) {
        // 读取的字节数不可能超过可读字节数
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            // len小于可读字节数，读索引向后移动
            readerIndex_ += len;
        } else {
            // len等于可读字节数，直接重置读写索引
            retrieveAll();
        }
    }

    // 检索了所有可读字符，直接重置读索引和写索引的位置到初始位置
    void retrieveAll() {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 检索所有可读字符，将其全部转换为string
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    // 检索len个可读字符，将其转换为string
    std::string retrieveAsString(size_t len) {
        // 要读取的字节数不可能超过可读字节数
        assert(len <= readableBytes());
        std::string result(peek(), len);
        // 更新读索引
        retrieve(len);
        return result;
    }

    // 确保buffer可以写入len个字节，可能触发扩容
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        // 获取空间后，可写字节数一定大于等于要写入的字节数
        assert(writableBytes() >= len);
    }

    // 获取写索引所在的指针
    char* beginWrite() {
        return begin() + writerIndex_;
    }
    // 获取写索引所在的指针
    const char* beginWrite() const {
        return begin() + writerIndex_;
    }

    // 已经写入len字节，写索引向后移动len
    void hasWritten(size_t len) {
        writerIndex_ += len;
    }

    // 将len字节的数据添加到buffer的尾部，可能触发扩容
    void append(const char* data, size_t len) {
        // 确保有足够空间写入
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        // 更新写索引
        hasWritten(len);
    }

    // 将一段string添加到buffer尾部，可能触发扩容
    void append(const std::string& str) {
        append(str.data(), str.size());
    }

    // 从fd读数据
    ssize_t readFd(int fd, int* savedErrno);

    // 向fd写数据
    ssize_t writeFd(int fd, int* savedErrno);

   private:
    // 获取buffer的起始指针
    char* begin() {
        return &*buffer_.begin();
    }

    // 获取buffer的起始指针
    const char* begin() const {
        return &*buffer_.begin();
    }

    // 根据传入的len判断是整理空间还是扩容
    void makeSpace(size_t len);

    // 使用vector<char>更契合TCP的流式传输
    std::vector<char> buffer_;
    // 读索引和写索引，将buffer划分为预留区，可读区，可写区
    // [预留区域][可读数据][可写区域]
    // 预留区域：readerIndex_ 之前的空间
    // 可读数据：readerIndex_ 到 writerIndex_ 之间的空间
    // 可写区域：writerIndex_ 到缓冲区末尾的空间
    size_t readerIndex_;
    size_t writerIndex_;
};
}  // namespace base