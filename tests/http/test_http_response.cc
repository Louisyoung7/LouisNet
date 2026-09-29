#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "net/http/HttpResponse.h"

using net::http::HttpResponse;

// 测试默认状态：版本 HTTP/1.1、状态码 200、无头无体，toString 输出仅状态行+空行
TEST(HttpResponseTest, DefaultState) {
    HttpResponse resp;
    EXPECT_EQ(resp.toString(), "HTTP/1.1 200 OK\r\n\r\n");
}

// 测试链式调用：各 setter 返回 HttpResponse&，可连续设置并正确输出
TEST(HttpResponseTest, ChainedSetters) {
    HttpResponse resp;
    resp.setVersion("HTTP/1.0").setStatusCode(404).setHeader("Content-Type", "text/html").setBody("hi");
    EXPECT_EQ(resp.toString(), "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\nhi");
}

// 测试常见状态码到状态文本的映射
TEST(HttpResponseTest, StatusMessages) {
    const struct {
        int code;
        const char* text;
    } cases[] = {
        {200, "OK"},
        {400, "Bad Request"},
        {404, "Not Found"},
        {500, "Internal Server Error"},
    };
    for (const auto& c : cases) {
        HttpResponse resp;
        resp.setStatusCode(c.code);
        EXPECT_EQ(resp.toString(), "HTTP/1.1 " + std::to_string(c.code) + " " + c.text + "\r\n\r\n");
    }
}

// 测试未知状态码回退为 "Unknown"
TEST(HttpResponseTest, UnknownStatusMessage) {
    HttpResponse resp;
    resp.setStatusCode(599);
    EXPECT_EQ(resp.toString(), "HTTP/1.1 599 Unknown\r\n\r\n");
}

// 测试响应头：std::map 按 key 字典序输出，同一 key 重复设置时后者覆盖前者
TEST(HttpResponseTest, HeaderOrderAndOverwrite) {
    HttpResponse resp;
    resp.setHeader("B-Header", "2").setHeader("A-Header", "1").setHeader("B-Header", "overwritten");
    EXPECT_EQ(resp.toString(), "HTTP/1.1 200 OK\r\nA-Header: 1\r\nB-Header: overwritten\r\n\r\n");
}

// 测试 setBody(const std::string&)：正确写入响应体
TEST(HttpResponseTest, BodyFromString) {
    HttpResponse resp;
    resp.setBody(std::string("hello"));
    EXPECT_EQ(resp.toString(), "HTTP/1.1 200 OK\r\n\r\nhello");
}

// 测试 setBody(const char*, len)：按指定长度截取，且保留含 '\0' 的二进制数据
TEST(HttpResponseTest, BodyFromCharPointerWithLength) {
    HttpResponse resp;
    const char data[] = {'a', '\0', 'b'};
    resp.setBody(data, sizeof(data));
    // 显式指定长度 22，避免期望值在 '\0' 处被截断
    EXPECT_EQ(resp.toString(), std::string("HTTP/1.1 200 OK\r\n\r\na\0b", 22));
}

// 测试 setBody(std::string&&)：移除重载被正确选择，内容完整转移
TEST(HttpResponseTest, BodyFromMovedString) {
    HttpResponse resp;
    std::string body = "moved body";
    resp.setBody(std::move(body));
    EXPECT_EQ(resp.toString(), "HTTP/1.1 200 OK\r\n\r\nmoved body");
}

// 测试响应体为空时不输出 body，报文以头后空行结束
TEST(HttpResponseTest, EmptyBody) {
    HttpResponse resp;
    resp.setStatusCode(204);
    EXPECT_EQ(resp.toString(), "HTTP/1.1 204 Unknown\r\n\r\n");
}

// 测试 reset：清空头部与响应体、状态码恢复 200，版本恢复为 HTTP/1.1
TEST(HttpResponseTest, ResetClearsState) {
    HttpResponse resp;
    resp.setVersion("HTTP/1.0").setStatusCode(404).setHeader("X-Key", "value").setBody("data");
    resp.reset();
    EXPECT_EQ(resp.toString(), "HTTP/1.1 200 OK\r\n\r\n");
}
