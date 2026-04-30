#include <gtest/gtest.h>

#include "base/Buffer.h"
#include "net/http/HttpParser.h"

using namespace base;
using namespace net::http;

class HttpParserBufferTest : public ::testing::Test {
   protected:
    void SetUp() override {}
    void TearDown() override {}

    Buffer buffer_;
    HttpParser parser_;
    HttpRequest request_;
};

TEST_F(HttpParserBufferTest, ParseSimpleGetRequest) {
    std::string httpRequest =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "GET");
    EXPECT_EQ(request_.path_, "/index.html");
    EXPECT_EQ(request_.version_, "HTTP/1.1");
    EXPECT_EQ(request_.headers_["host"], "localhost");
    EXPECT_TRUE(request_.body_.empty());
}

TEST_F(HttpParserBufferTest, ParseGetRequestWithMultipleHeaders) {
    std::string httpRequest =
        "GET /api/users HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Accept: application/json\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "\r\n";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "GET");
    EXPECT_EQ(request_.path_, "/api/users");
    EXPECT_EQ(request_.version_, "HTTP/1.1");
    EXPECT_EQ(request_.headers_["host"], "localhost:8080");
    EXPECT_EQ(request_.headers_["accept"], "application/json");
    EXPECT_EQ(request_.headers_["user-agent"], "TestClient/1.0");
}

TEST_F(HttpParserBufferTest, ParsePostRequestWithBody) {
    std::string httpRequest =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello World";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "POST");
    EXPECT_EQ(request_.path_, "/submit");
    EXPECT_EQ(request_.version_, "HTTP/1.1");
    EXPECT_EQ(request_.headers_["content-length"], "11");
    EXPECT_EQ(request_.body_, "Hello World");
}

TEST_F(HttpParserBufferTest, ParseRequestPartialData) {
    std::string httpRequest =
        "GET /test HTTP/1.1\r\n"
        "Host: localhost\r\n";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kNeedMore);
    EXPECT_EQ(request_.method_, "GET");
    EXPECT_EQ(request_.path_, "/test");
}

TEST_F(HttpParserBufferTest, ParseRequestInMultipleChunks) {
    parser_.reset();
    request_.reset();

    buffer_.append("GET /chunked");
    auto result = parser_.parseRequest(buffer_, request_);
    EXPECT_EQ(result, HttpParser::ParseResult::kNeedMore);

    buffer_.append(" HTTP/1.1\r\n");
    result = parser_.parseRequest(buffer_, request_);
    EXPECT_EQ(result, HttpParser::ParseResult::kNeedMore);

    buffer_.append("Host: localhost\r\n");
    result = parser_.parseRequest(buffer_, request_);
    EXPECT_EQ(result, HttpParser::ParseResult::kNeedMore);

    buffer_.append("\r\n");
    result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "GET");
    EXPECT_EQ(request_.path_, "/chunked");
    EXPECT_EQ(request_.version_, "HTTP/1.1");
    EXPECT_EQ(request_.headers_["host"], "localhost");
}

TEST_F(HttpParserBufferTest, ParsePostRequestBodyInChunks) {
    std::string part1 =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 23\r\n"
        "\r\n"
        "First 10 bytes";

    std::string part2 = " and more";

    buffer_.append(part1);
    auto result = parser_.parseRequest(buffer_, request_);
    EXPECT_EQ(result, HttpParser::ParseResult::kNeedMore);
    EXPECT_EQ(request_.body_, "First 10 bytes");

    buffer_.append(part2);
    result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.body_, "First 10 bytes and more");
}

TEST_F(HttpParserBufferTest, ParseEmptyBodyPostRequest) {
    std::string httpRequest =
        "POST /empty HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "POST");
    EXPECT_EQ(request_.path_, "/empty");
    EXPECT_TRUE(request_.body_.empty());
}

TEST_F(HttpParserBufferTest, ParseRequestWithoutContentLength) {
    std::string httpRequest =
        "GET /no-body HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "GET");
    EXPECT_TRUE(request_.body_.empty());
}

TEST_F(HttpParserBufferTest, MultipleRequestsSequential) {
    std::string httpRequest1 =
        "GET /first HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string httpRequest2 =
        "POST /second HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 4\r\n"
        "\r\n"
        "data";

    buffer_.append(httpRequest1);
    auto result = parser_.parseRequest(buffer_, request_);
    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "GET");
    EXPECT_EQ(request_.path_, "/first");

    parser_.reset();
    request_.reset();

    buffer_.append(httpRequest2);
    result = parser_.parseRequest(buffer_, request_);
    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.method_, "POST");
    EXPECT_EQ(request_.path_, "/second");
    EXPECT_EQ(request_.body_, "data");
}

TEST_F(HttpParserBufferTest, HeaderKeyCaseInsensitive) {
    std::string httpRequest =
        "GET /test HTTP/1.1\r\n"
        "HOST: localhost\r\n"
        "Content-Type: text/html\r\n"
        "ACCEPT: */*\r\n"
        "\r\n";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.headers_["host"], "localhost");
    EXPECT_EQ(request_.headers_["content-type"], "text/html");
    EXPECT_EQ(request_.headers_["accept"], "*/*");
}

TEST_F(HttpParserBufferTest, HeaderValueWithLeadingSpaces) {
    std::string httpRequest =
        "GET /test HTTP/1.1\r\n"
        "X-Custom:   value with spaces  \r\n"
        "\r\n";

    buffer_.append(httpRequest);
    auto result = parser_.parseRequest(buffer_, request_);

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);
    EXPECT_EQ(request_.headers_["x-custom"], "value with spaces");
}