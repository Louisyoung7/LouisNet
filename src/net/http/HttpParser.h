#pragma once

#include "HttpRequest.h"

namespace base {
class Buffer;
}  // namespace base

namespace net::http {

class HttpParser {
   public:
    enum class ParseResult {
        kNeedMore,  // 需要更多数据
        kSuccess,   // 解析成功
        kError,     // 解析错误
    };

    enum class State {
        kStartLine,  // 开始解析
        kHeaders,    // 头
        kBody,       // 消息体
    };


    // 解析HTTP请求
    ParseResult parseRequest(base::Buffer& buffer, HttpRequest& outRequest);

    // 重置解析器
    void reset();

   private:
    State state_ = State::kStartLine;         // 当前解析状态
    size_t contentLength_ = 0;                // 当前解析的请求体长度
    size_t bodyReceived_ = 0;                 // 已接收的请求体长度

    // 解析C字符串
    ParseResult parseCString(const char* data, size_t len, HttpRequest& outRequest, size_t& parsed);
};
}  // namespace net::http
