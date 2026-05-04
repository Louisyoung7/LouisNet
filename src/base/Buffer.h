#pragma once

#include <arpa/inet.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace base {
class Buffer {
    // 使用vector<char>更契合TCP的流式传输
    std::vector<char> buffer_;
    // 读索引和写索引，将buffer划分为预留区，可读区，可写区
    // [预留区域][可读数据][可写区域]
    // 预留区域：readerIndex_ 之前的空间
    // 可读数据：readerIndex_ 到 writerIndex_ 之间的空间
    // 可写区域：writerIndex_ 到缓冲区末尾的空间
    size_t readerIndex_;
    size_t writerIndex_;

   public:
    // 预留8字节，方便添加协议头部
    static constexpr size_t kCheapPrepend = 8;
    // 默认的缓冲区初始大小1kb
    static constexpr size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {}

    // 交换两个缓冲区的内容
    void swap(Buffer& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(readerIndex_, other.readerIndex_);
        std::swap(writerIndex_, other.writerIndex_);
    }

    /// 获取可读、可写和预留字节数量

    // 获取可读字节数，写索引位置减去读索引位置
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    // 获取可写字节数，buffer现有容量减去写索引位置
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    // 返回预留的字节数
    //* 这里直接返回读索引位置，读索引一定大于等于预留字节数，这样协议头直接添加，不用专门移动数据
    size_t prependableBytes() const { return readerIndex_; }

    // 获取读索引所在的指针
    const char* peek() const { return begin() + readerIndex_; }
    // 获取读索引所在的指针
    char* peek() { return begin() + readerIndex_; }

    /// 消费可读字节

    // 消费len个可读字节，读索引向后移动len
    void consume(size_t len) {
        // 消费的字节数不可能超过可读字节数
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            // len小于可读字节数，读索引向后移动
            readerIndex_ += len;
        } else {
            // len等于可读字节数，直接重置读写索引
            consumeAll();
        }
    }

    // 消费所有可读字节，直接重置读索引和写索引的位置到初始位置
    void consumeAll() { readerIndex_ = writerIndex_ = kCheapPrepend; }

    // 消费int32_t类型的可读字节
    void consumeInt32() { consume(sizeof(int32_t)); }

    /// 检索并消费可读字节

    // 检索并消费int32_t类型的可读字节，将其转换为大端序
    int32_t retrieveInt32() {
        // 检查是否有足够的字节可读
        assert(readableBytes() >= sizeof(int32_t));
        // 从读索引开始读取int32_t类型的数
        int32_t x{};
        ::memcpy(&x, peek(), sizeof(int32_t));
        // 更新读索引
        consumeInt32();
        // 转换为大端序
        x = ::ntohl(x);
        // 返回int32_t类型的数
        return x;
    }

    // 检索并消费所有可读字符，将其全部转换为string
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }

    // 检索并消费len个可读字符，将其转换为string
    std::string retrieveAsString(size_t len) {
        // 要读取的字节数不可能超过可读字节数
        assert(len <= readableBytes());
        std::string result(peek(), len);
        // 更新读索引
        consume(len);
        return result;
    }

    // 确保buffer可以写入len个字节，可能触发扩容
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) { makeSpace(len); }
        // 获取空间后，可写字节数一定大于等于要写入的字节数
        assert(writableBytes() >= len);
    }

    // 获取写索引所在的指针
    char* writerIt() { return begin() + writerIndex_; }
    // 获取写索引所在的指针
    const char* writerIt() const { return begin() + writerIndex_; }

    // 已经写入len字节，写索引向后移动len
    void hasWritten(size_t len) { writerIndex_ += len; }

    /// 添加数据到buffer尾部

    // 添加string到buffer尾部，可能触发扩容
    void append(const std::string& str) { append(str.data(), str.size()); }

    // 将len字节数据添加到buffer的尾部，可能触发扩容
    void append(const void* data, size_t len) { append(reinterpret_cast<const char*>(data), len); }

    // 将len字节字符添加到buffer的尾部，可能触发扩容
    void append(const char* data, size_t len) {
        // 确保有足够空间写入
        ensureWritableBytes(len);
        std::copy(data, data + len, writerIt());
        // 更新写索引
        hasWritten(len);
    }

    // 添加int32_t类型的数据到buffer尾部，可能触发扩容
    void appendInt32(int32_t x) {
        // 转换为网络字节序
        x = ::htonl(x);
        append(&x, sizeof(int32_t));
    }

    // 读索引向前移动len，将len字节数据复制到buffer的头部
    void prepend(const void* data, size_t len) {
        assert(len <= prependableBytes());
        // 读索引向前移动len
        readerIndex_ -= len;
        // 复制数据到buffer的头部
        const char* prependData = reinterpret_cast<const char*>(data);
        std::copy(prependData, prependData + len, peek());
    }

    void prependInt32(int32_t x) {
        // 转换为网络字节序
        x = ::htonl(x);
        prepend(&x, sizeof(int32_t));
    }

    // 从fd读数据
    ssize_t readFd(int fd, int* savedErrno);

   private:
    // 获取buffer的起始指针
    char* begin() { return &*buffer_.begin(); }
    // 获取buffer的起始指针
    const char* begin() const { return &*buffer_.begin(); }

    // 根据传入的len判断是整理空间还是扩容
    void makeSpace(size_t len);
};
}  // namespace base