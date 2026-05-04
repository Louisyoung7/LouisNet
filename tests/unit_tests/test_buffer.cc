#include <gtest/gtest.h>

#include "base/Buffer.h"

class BufferTest : public ::testing::Test {
   protected:
    void SetUp() override {}

    void TearDown() override {}

    base::Buffer buffer_;
};

// 测试写入和读取的基本功能
TEST_F(BufferTest, BasicWriteRead) {
    const std::string test_str = "Hello, Buffer!";
    buffer_.append(test_str);
    EXPECT_EQ(buffer_.readableBytes(), test_str.size());
    EXPECT_EQ(buffer_.retrieveAllAsString(), test_str);
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试多次写入和读取
TEST_F(BufferTest, MutipleWriteRead) {
    const std::string str1 = "Hello";
    const std::string str2 = " World";
    const std::string expected = str1 + str2;

    buffer_.append(str1);
    buffer_.append(str2);

    EXPECT_EQ(buffer_.readableBytes(), expected.size());
    EXPECT_EQ(buffer_.retrieveAllAsString(), expected);
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试部分读取
TEST_F(BufferTest, PartialRead) {
    const std::string test_str = "Hello, Buffer";
    buffer_.append(test_str);

    const size_t read_len = 5;

    std::string partial = buffer_.retrieveAsString(read_len);
    EXPECT_EQ(partial, test_str.substr(0, read_len));
    EXPECT_EQ(buffer_.readableBytes(), test_str.size() - read_len);

    std::string remaining = buffer_.retrieveAllAsString();
    EXPECT_EQ(remaining, test_str.substr(read_len));
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试缓冲区的边界情况
TEST_F(BufferTest, Boundary) {
    // 空缓冲区测试
    EXPECT_EQ(buffer_.readableBytes(), 0);
    EXPECT_EQ(buffer_.writableBytes(), base::Buffer::kInitialSize);
    EXPECT_EQ(buffer_.retrieveAllAsString(), "");

    // 大量数据测试
    const std::string large_str(10000, 'a');
    buffer_.append(large_str);
    EXPECT_EQ(buffer_.readableBytes(), large_str.size());
    EXPECT_EQ(buffer_.retrieveAllAsString(), large_str);
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试内部缓冲区大小调整
TEST_F(BufferTest, BufferResize) {
    const size_t initial_size = buffer_.writableBytes();
    EXPECT_GT(initial_size, 0);

    // 写入超过初始大小的数据
    const std::string large_str(initial_size * 2, 'a');
    buffer_.append(large_str);

    // 缓冲区应该自动扩展
    EXPECT_EQ(buffer_.readableBytes(), large_str.size());
    EXPECT_EQ(buffer_.retrieveAllAsString(), large_str);
    EXPECT_EQ(buffer_.readableBytes(), 0);

    // 读取数据后，缓冲区大小应该保持合理
    EXPECT_GE(buffer_.writableBytes(), base::Buffer::kInitialSize);
}

// 测试 consumeAll 功能
TEST_F(BufferTest, ConsumeAll) {
    const std::string test_str = "Test consumeAll";
    buffer_.append(test_str);
    EXPECT_EQ(buffer_.readableBytes(), test_str.size());

    buffer_.consumeAll();
    EXPECT_EQ(buffer_.readableBytes(), 0);
    EXPECT_EQ(buffer_.writableBytes(), base::Buffer::kInitialSize);
}

// 测试 consume 部分消费功能
TEST_F(BufferTest, Consume) {
    const std::string test_str = "Hello, World!";
    buffer_.append(test_str);
    EXPECT_EQ(buffer_.readableBytes(), test_str.size());

    buffer_.consume(5);
    EXPECT_EQ(buffer_.readableBytes(), test_str.size() - 5);
    EXPECT_EQ(buffer_.retrieveAllAsString(), ", World!");

    buffer_.consumeAll();
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试 prependableBytes 功能
TEST_F(BufferTest, PrependableBytes) {
    EXPECT_EQ(buffer_.prependableBytes(), base::Buffer::kCheapPrepend);

    const std::string test_str = "Test";
    buffer_.append(test_str);
    EXPECT_EQ(buffer_.prependableBytes(), base::Buffer::kCheapPrepend);

    buffer_.consume(2);
    EXPECT_EQ(buffer_.prependableBytes(), base::Buffer::kCheapPrepend + 2);
}

// 测试 peek 和 writerIt 功能
TEST_F(BufferTest, PeekAndWriterIt) {
    const std::string test_str = "Hello";
    buffer_.append(test_str);

    const char* peek_ptr = buffer_.peek();
    EXPECT_EQ(memcmp(peek_ptr, test_str.data(), test_str.size()), 0);

    const char* writer_ptr = buffer_.writerIt();
    EXPECT_EQ(*writer_ptr, '\0');
}

// 测试 hasWritten 功能
TEST_F(BufferTest, HasWritten) {
    buffer_.append("Hello");
    EXPECT_EQ(buffer_.readableBytes(), 5);

    buffer_.hasWritten(3);
    EXPECT_EQ(buffer_.readableBytes(), 8);
}

// 测试 ensureWritableBytes 功能
TEST_F(BufferTest, EnsureWritableBytes) {
    const size_t initial_writable = buffer_.writableBytes();
    buffer_.ensureWritableBytes(initial_writable + 100);

    EXPECT_GE(buffer_.writableBytes(), initial_writable + 100);
}

// 测试 swap 功能
TEST_F(BufferTest, Swap) {
    const std::string str1 = "Buffer1";
    const std::string str2 = "Buffer2";

    base::Buffer buffer1;
    buffer1.append(str1);

    base::Buffer buffer2;
    buffer2.append(str2);

    const size_t buffer1_readable = buffer1.readableBytes();
    const size_t buffer2_readable = buffer2.readableBytes();

    buffer1.swap(buffer2);

    EXPECT_EQ(buffer1.readableBytes(), buffer2_readable);
    EXPECT_EQ(buffer2.readableBytes(), buffer1_readable);
    EXPECT_EQ(buffer1.retrieveAllAsString(), str2);
    EXPECT_EQ(buffer2.retrieveAllAsString(), str1);
}

// 测试 int32 类型的追加和检索（网络字节序）
TEST_F(BufferTest, Int32Retrieve) {
    int32_t test_val = 0x12345678;
    buffer_.appendInt32(test_val);
    EXPECT_EQ(buffer_.readableBytes(), sizeof(int32_t));

    int32_t retrieved = buffer_.retrieveInt32();
    EXPECT_EQ(retrieved, test_val);
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试 consumeInt32 功能
TEST_F(BufferTest, ConsumeInt32) {
    const std::string str = "ABCD";
    buffer_.append(str);
    EXPECT_EQ(buffer_.readableBytes(), 4);

    buffer_.consumeInt32();
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试多次 append 后 int32 读取正确性
TEST_F(BufferTest, AppendInt32AndRetrieve) {
    int32_t val1 = 0x12345678;
    int32_t val2 = 0xABCDEF00;

    buffer_.appendInt32(val1);
    buffer_.append("extra");
    buffer_.appendInt32(val2);

    EXPECT_EQ(buffer_.readableBytes(), sizeof(int32_t) * 2 + 5);

    int32_t r1 = buffer_.retrieveInt32();
    EXPECT_EQ(r1, val1);

    buffer_.consume(5);

    int32_t r2 = buffer_.retrieveInt32();
    EXPECT_EQ(r2, val2);
    EXPECT_EQ(buffer_.readableBytes(), 0);
}

// 测试 prepend 功能
TEST_F(BufferTest, Prepend) {
    const std::string test_str = "World";
    buffer_.append(test_str);
    EXPECT_EQ(buffer_.readableBytes(), 5);

    const std::string prepend_str = "Hello";
    buffer_.prepend(prepend_str.data(), prepend_str.size());

    EXPECT_EQ(buffer_.readableBytes(), 5 + prepend_str.size());
    EXPECT_EQ(buffer_.retrieveAllAsString(), "HelloWorld");
}

// 测试 prependInt32 功能
TEST_F(BufferTest, PrependInt32) {
    int32_t test_val = 0x12345678;
    buffer_.append("Hello");

    buffer_.prependInt32(test_val);
    EXPECT_EQ(buffer_.readableBytes(), 4 + 5);

    int32_t retrieved = buffer_.retrieveInt32();
    EXPECT_EQ(retrieved, test_val);
    EXPECT_EQ(buffer_.retrieveAllAsString(), "Hello");
}

// 测试 prepend 后 consume 再 prepend 的场景
TEST_F(BufferTest, PrependAfterConsume) {
    buffer_.append("World");
    buffer_.consume(3);

    buffer_.prepend("X", 1);
    EXPECT_EQ(buffer_.retrieveAllAsString(), "Xld");
}