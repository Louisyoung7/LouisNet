#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "base/Buffer.h"
#include "net/http/HttpParser.h"
#include "net/http/HttpRequest.h"

using net::http::HttpParser;
using net::http::HttpRequest;
using ParseResult = HttpParser::ParseResult;

namespace {
// 构造一个已写入 str 的缓冲区
base::Buffer makeBuffer(const std::string& str) {
    base::Buffer buf;
    buf.append(str);
    return buf;
}
}  // namespace

// ======================== 边界类 ========================

// 测试空缓冲区：返回 kNeedMore，不消费任何数据
TEST(HttpParserBoundary, EmptyBuffer) {
    base::Buffer buf;
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kNeedMore);
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试最小合法请求：GET 无头无体，状态行后直接空行即成功，缓冲区被消费完
TEST(HttpParserBoundary, MinimalGetRequest) {
    base::Buffer buf = makeBuffer("GET / HTTP/1.1\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_EQ(req.method_, "GET");
    EXPECT_EQ(req.path_, "/");
    EXPECT_EQ(req.version_, "HTTP/1.1");
    EXPECT_TRUE(req.headers_.empty());
    EXPECT_TRUE(req.body_.empty());
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试请求行缺少空格（无法拆出方法/路径/版本）→ kError
TEST(HttpParserBoundary, RequestLineWithoutSpace) {
    base::Buffer buf = makeBuffer("GETHTTP/1.1\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kError);
    // 出错时缓冲区全部丢弃
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试报文以空行开头（空请求行）→ kError
TEST(HttpParserBoundary, LeadingCRLF) {
    base::Buffer buf = makeBuffer("\r\nGET / HTTP/1.1\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kError);
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试头部行缺少冒号 → kError
TEST(HttpParserBoundary, HeaderWithoutColon) {
    base::Buffer buf = makeBuffer("GET / HTTP/1.1\r\nBadHeader\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kError);
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试头部规范化：头部名转小写、值两端空格裁剪、冒号后无空格也兼容
TEST(HttpParserBoundary, HeaderKeyNormalization) {
    base::Buffer buf = makeBuffer("GET / HTTP/1.1\r\nHost:  Example.com  \r\nX-Token:abc\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    ASSERT_EQ(req.headers_.size(), 2u);
    EXPECT_EQ(req.headers_["host"], "Example.com");
    EXPECT_EQ(req.headers_["x-token"], "abc");
}

// 测试 Content-Length: 0：头部结束后立即成功，body 为空
TEST(HttpParserBoundary, ContentLengthZero) {
    base::Buffer buf = makeBuffer("POST /a HTTP/1.1\r\nContent-Length: 0\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_TRUE(req.body_.empty());
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试 Content-Length 非数字 → kError
TEST(HttpParserBoundary, InvalidContentLength) {
    base::Buffer buf = makeBuffer("POST /a HTTP/1.1\r\nContent-Length: abc\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kError);
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试 body 含 '\0' 的二进制数据按 Content-Length 原样保留
TEST(HttpParserBoundary, BinaryBodyPreserved) {
    base::Buffer buf = makeBuffer("POST /up HTTP/1.1\r\nContent-Length: 4\r\n\r\n");
    buf.append(std::string("a\0b\0", 4));
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_EQ(req.body_, std::string("a\0b\0", 4));
}

// ======================== 时序类 ========================

// 测试逐字节到达：数据不完整时始终 kNeedMore，最后一个字节到达后 kSuccess
TEST(HttpParserSequence, ByteByByteFeed) {
    const std::string raw = "GET /idx HTTP/1.1\r\nHost: h\r\n\r\n";
    base::Buffer buf;
    HttpRequest req;
    HttpParser parser;

    for (size_t i = 0; i < raw.size(); ++i) {
        buf.append(std::string(1, raw[i]));
        ParseResult result = parser.parseRequest(buf, req);
        if (i + 1 < raw.size()) {
            EXPECT_EQ(result, ParseResult::kNeedMore) << "第 " << i + 1 << " 个字节后应仍需更多数据";
        } else {
            EXPECT_EQ(result, ParseResult::kSuccess);
        }
    }
    EXPECT_EQ(req.method_, "GET");
    EXPECT_EQ(req.path_, "/idx");
    EXPECT_EQ(req.headers_["host"], "h");
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试头部行被拆到两次数据中：半行数据不消费（留在缓冲区），拼齐后解析成功
TEST(HttpParserSequence, HeaderSplitAcrossChunks) {
    base::Buffer buf = makeBuffer("GET / HTTP/1.1\r\nHost: exam");
    HttpRequest req;
    HttpParser parser;

    // 请求行已消费，半行头部留在缓冲区
    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kNeedMore);
    EXPECT_EQ(buf.readableBytes(), std::strlen("Host: exam"));

    // 补齐头部后成功
    buf.append("ple.com\r\n\r\n");
    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_EQ(req.headers_["host"], "example.com");
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// 测试 body 分片到达：分片间 kNeedMore 且 body 逐段累计，收齐后成功
TEST(HttpParserSequence, BodySplitAcrossChunks) {
    base::Buffer buf = makeBuffer("POST /echo HTTP/1.1\r\nContent-Length: 10\r\n\r\n0123");
    HttpRequest req;
    HttpParser parser;

    // 头部消费完，body 前半段进入请求对象但整体未完成
    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kNeedMore);
    EXPECT_EQ(req.body_, "0123");
    EXPECT_EQ(buf.readableBytes(), 0u);

    // 剩余 body 到达，累计完整
    buf.append("456789");
    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_EQ(req.body_, "0123456789");
}

// 测试管线化：缓冲区含两个完整请求，第一次解析只消费第一个，
// 第二个请求留在缓冲区；带 body 的成功路径内部已复位，可继续用同一解析器
TEST(HttpParserSequence, PipelinedTwoRequests) {
    const std::string req2Raw = "GET /2 HTTP/1.1\r\n\r\n";
    base::Buffer buf = makeBuffer("POST /1 HTTP/1.1\r\nContent-Length: 2\r\n\r\nab" + req2Raw);
    HttpRequest req;
    HttpParser parser;

    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_EQ(req.path_, "/1");
    EXPECT_EQ(req.body_, "ab");
    // 第二个请求完整保留在缓冲区
    EXPECT_EQ(buf.readableBytes(), req2Raw.size());

    // 同一解析器继续解析第二个请求
    req.reset();
    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_EQ(req.method_, "GET");
    EXPECT_EQ(req.path_, "/2");
    EXPECT_EQ(buf.readableBytes(), 0u);
}

// ======================== 生命周期类 ========================

// 测试无 body 请求解析成功后不复位：状态停留在 kHeaders，
// 不 reset 直接复用会把下一个请求行当头部解析而报错（记录当前行为，
// HttpServer 通过 resetContext() 显式规避）
TEST(HttpParserLifecycle, ReuseBodylessWithoutResetIsError) {
    base::Buffer buf1 = makeBuffer("GET / HTTP/1.1\r\n\r\n");
    HttpRequest req1;
    HttpParser parser;
    ASSERT_EQ(parser.parseRequest(buf1, req1), ParseResult::kSuccess);

    base::Buffer buf2 = makeBuffer("GET / HTTP/1.1\r\n\r\n");
    HttpRequest req2;
    EXPECT_EQ(parser.parseRequest(buf2, req2), ParseResult::kError);
    // 出错路径清空缓冲并自动复位
    EXPECT_EQ(buf2.readableBytes(), 0u);
}

// 测试显式 reset 后复用：模拟 HttpServer 的用法，请求间 reset 可正常连续解析
TEST(HttpParserLifecycle, ExplicitResetBetweenRequests) {
    HttpParser parser;
    HttpRequest req;

    base::Buffer buf1 = makeBuffer("GET /a HTTP/1.1\r\n\r\n");
    ASSERT_EQ(parser.parseRequest(buf1, req), ParseResult::kSuccess);

    parser.reset();  // 请求间显式复位
    base::Buffer buf2 = makeBuffer("POST /b HTTP/1.1\r\nContent-Length: 1\r\n\r\nX");
    ASSERT_EQ(parser.parseRequest(buf2, req), ParseResult::kSuccess);
    EXPECT_EQ(req.method_, "POST");
    EXPECT_EQ(req.path_, "/b");
    EXPECT_EQ(req.body_, "X");
}

// 测试解析中途 reset 丢弃半成品：body 分片解析到一半时 reset，
// 解析器回到初始状态，可对新的完整请求正常解析
TEST(HttpParserLifecycle, ResetDiscardsPartialProgress) {
    base::Buffer buf = makeBuffer("POST /u HTTP/1.1\r\nContent-Length: 6\r\n\r\nabc");
    HttpRequest req;
    HttpParser parser;

    // body 只到一半，kNeedMore，解析器处于 kBody 且已累计部分 body
    ASSERT_EQ(parser.parseRequest(buf, req), ParseResult::kNeedMore);

    // 丢弃半成品（调用方应同时换用/重置请求对象）
    parser.reset();
    HttpRequest freshReq;
    base::Buffer buf2 = makeBuffer("GET /ok HTTP/1.1\r\n\r\n");
    EXPECT_EQ(parser.parseRequest(buf2, freshReq), ParseResult::kSuccess);
    EXPECT_EQ(freshReq.path_, "/ok");
}

// 测试出错自恢复：kError 后缓冲区被清空、解析器自动复位，
// 随后无需手动 reset 即可解析新请求
TEST(HttpParserLifecycle, ErrorAutoRecovery) {
    base::Buffer buf = makeBuffer("GARBAGE\r\n\r\n");
    HttpRequest req;
    HttpParser parser;

    ASSERT_EQ(parser.parseRequest(buf, req), ParseResult::kError);
    EXPECT_EQ(buf.readableBytes(), 0u);

    buf.append("GET /ok HTTP/1.1\r\n\r\n");
    EXPECT_EQ(parser.parseRequest(buf, req), ParseResult::kSuccess);
    EXPECT_EQ(req.path_, "/ok");
}

// ======================== 并发类 ========================

// HttpParser 无同步措施，单个实例非线程安全，禁止跨线程共享；
// 典型并发用法是"每连接一个解析器"：多线程各自持有独立的
// 解析器/缓冲区/请求对象并行解析，互不干扰
TEST(HttpParserConcurrency, PerConnectionParsers) {
    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::vector<ParseResult> results(kThreads);
    std::vector<std::string> paths(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i, &results, &paths] {
            HttpParser parser;  // 每线程独立实例
            base::Buffer buf;
            buf.append("GET /t" + std::to_string(i) + " HTTP/1.1\r\n\r\n");
            HttpRequest req;
            results[i] = parser.parseRequest(buf, req);
            paths[i] = req.path_;
        });
    }
    for (auto& t : threads) t.join();

    for (int i = 0; i < kThreads; ++i) {
        EXPECT_EQ(results[i], ParseResult::kSuccess) << "线程 " << i << " 解析失败";
        EXPECT_EQ(paths[i], "/t" + std::to_string(i));
    }
}
