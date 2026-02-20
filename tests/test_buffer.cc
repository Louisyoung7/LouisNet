#include <gtest/gtest.h>
#include <cstddef>

#include "utils/Buffer.h"

class BufferTest : public ::testing::Test {
   protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    utils::Buffer buffer_;
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
    EXPECT_EQ(buffer_.writableBytes(), utils::Buffer::kInitialSize);
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
    EXPECT_GE(buffer_.writableBytes(), utils::Buffer::kInitialSize);
}