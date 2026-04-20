#include <gtest/gtest.h>

#include "net/http/HttpParser.h"

using namespace net::http;

class HttpParserTest : public ::testing::Test {
   protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

    HttpParser parser_;
};

TEST_F(HttpParserTest, ParseBasicPostRequest) {
    std::string request_data =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 15\r\n"
        "\r\n"
        "name=test&age=1";

    auto result = parser_.parse(request_data.data(), request_data.size());

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);

    const auto& request = parser_.request();
    EXPECT_EQ(request.method_, "POST");
    EXPECT_EQ(request.path_, "/submit");
    EXPECT_EQ(request.version_, "HTTP/1.1");
    EXPECT_EQ(request.headers_.at("host"), "localhost:8080");
    EXPECT_EQ(request.headers_.at("content-type"), "application/x-www-form-urlencoded");
    EXPECT_EQ(request.headers_.at("content-length"), "15");
    EXPECT_EQ(request.body_, "name=test&age=1");
}

TEST_F(HttpParserTest, ParsePostRequestWithJsonBody) {
    std::string request_data =
        "POST /api/user HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"username\":\"test\",\"age\":1}";

    auto result = parser_.parse(request_data.data(), request_data.size());

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);

    const auto& request = parser_.request();
    EXPECT_EQ(request.method_, "POST");
    EXPECT_EQ(request.path_, "/api/user");
    EXPECT_EQ(request.version_, "HTTP/1.1");
    EXPECT_EQ(request.headers_.at("content-type"), "application/json");
    EXPECT_EQ(request.body_, "{\"username\":\"test\",\"age\":1}");
}

TEST_F(HttpParserTest, ParsePostRequestEmptyBody) {
    std::string request_data =
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    auto result = parser_.parse(request_data.data(), request_data.size());

    EXPECT_EQ(result, HttpParser::ParseResult::kSuccess);

    const auto& request = parser_.request();
    EXPECT_EQ(request.method_, "POST");
    EXPECT_EQ(request.path_, "/upload");
    EXPECT_EQ(request.body_, "");
}