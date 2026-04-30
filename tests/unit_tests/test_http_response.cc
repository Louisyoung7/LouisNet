#include <gtest/gtest.h>

#include "net/http/HttpResponse.h"

using namespace net::http;

class HttpResponseTest : public ::testing::Test {
   protected:
    void SetUp() override {}

    void TearDown() override {}

    HttpResponse response_;
};

TEST_F(HttpResponseTest, DefaultValues) {
    std::string expected = "HTTP/1.1 200 OK\r\n\r\n";
    EXPECT_EQ(response_.toString(), expected);
}

TEST_F(HttpResponseTest, SetVersion) {
    response_.setVersion("HTTP/1.0");
    std::string result = response_.toString();
    EXPECT_TRUE(result.find("HTTP/1.0 200 OK") == 0);
}

TEST_F(HttpResponseTest, SetStatusCode) {
    response_.setStatusCode(404);
    std::string result = response_.toString();
    EXPECT_TRUE(result.find("HTTP/1.1 404 Not Found") == 0);
}

TEST_F(HttpResponseTest, SetHeader) {
    response_.setHeader("Content-Type", "application/json");
    std::string result = response_.toString();
    EXPECT_TRUE(result.find("Content-Type: application/json\r\n") != std::string::npos);
}

TEST_F(HttpResponseTest, SetBodyString) {
    std::string body = "Hello, World!";
    response_.setBody(body);
    std::string result = response_.toString();
    EXPECT_TRUE(result.find("\r\n\r\nHello, World!") != std::string::npos);
}

TEST_F(HttpResponseTest, SetBodyCString) {
    const char* body = "Hello, C++!";
    response_.setBody(body, 11);
    std::string result = response_.toString();
    EXPECT_TRUE(result.find("\r\n\r\nHello, C++!") != std::string::npos);
}

TEST_F(HttpResponseTest, SetBodyRvalue) {
    response_.setBody(std::string("Rvalue Body"));
    std::string result = response_.toString();
    EXPECT_TRUE(result.find("\r\n\r\nRvalue Body") != std::string::npos);
}

TEST_F(HttpResponseTest, CompleteResponse) {
    response_.setVersion("HTTP/1.1");
    response_.setStatusCode(200);
    response_.setHeader("Content-Type", "application/json");
    response_.setHeader("Content-Length", "27");
    response_.setBody("{\"username\":\"test\",\"age\":1}");

    std::string result = response_.toString();

    EXPECT_TRUE(result.find("HTTP/1.1 200 OK\r\n") == 0);
    EXPECT_TRUE(result.find("Content-Type: application/json\r\n") != std::string::npos);
    EXPECT_TRUE(result.find("Content-Length: 27\r\n") != std::string::npos);
    EXPECT_TRUE(result.find("\r\n\r\n{\"username\":\"test\",\"age\":1}") != std::string::npos);
}

TEST_F(HttpResponseTest, NotFoundResponse) {
    response_.setStatusCode(404);
    response_.setHeader("Content-Type", "text/plain");
    response_.setBody("404 Not Found");

    std::string result = response_.toString();

    EXPECT_TRUE(result.find("HTTP/1.1 404 Not Found\r\n") == 0);
    EXPECT_TRUE(result.find("Content-Type: text/plain\r\n") != std::string::npos);
    EXPECT_TRUE(result.find("\r\n\r\n404 Not Found") != std::string::npos);
}

TEST_F(HttpResponseTest, ChainedResponse) {
    response_.setVersion("HTTP/1.1")
        .setStatusCode(200)
        .setHeader("Content-Type", "application/json")
        .setHeader("Content-Length", "27")
        .setBody("{\"username\":\"test\",\"age\":1}");

    std::string result = response_.toString();

    EXPECT_TRUE(result.find("HTTP/1.1 200 OK\r\n") == 0);
    EXPECT_TRUE(result.find("Content-Type: application/json\r\n") != std::string::npos);
    EXPECT_TRUE(result.find("Content-Length: 27\r\n") != std::string::npos);
    EXPECT_TRUE(result.find("\r\n\r\n{\"username\":\"test\",\"age\":1}") != std::string::npos);
}

TEST_F(HttpResponseTest, ChainedHeaders) {
    response_.setHeader("Header1", "Value1").setHeader("Header2", "Value2").setHeader("Header3", "Value3");

    std::string result = response_.toString();

    EXPECT_TRUE(result.find("Header1: Value1\r\n") != std::string::npos);
    EXPECT_TRUE(result.find("Header2: Value2\r\n") != std::string::npos);
    EXPECT_TRUE(result.find("Header3: Value3\r\n") != std::string::npos);
}

TEST_F(HttpResponseTest, ChainedSetBody) {
    std::string body = "Test body";
    response_.setBody(body).setBody("Direct string").setBody("Another body", 12);

    std::string result = response_.toString();

    EXPECT_TRUE(result.find("\r\n\r\nAnother body") != std::string::npos);
}
