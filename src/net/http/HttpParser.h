#pragma once

#include <string>
#include <string_view>

#include "HttpRequest.h"

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
    ParseResult parse(const std::string& data) {
        return parse(data.c_str(), data.size());
    }
    ParseResult parse(const char* data, size_t len);

    // 重置解析状态
    void reset();

    // 获取当前解析完成的请求对象
    const HttpRequest& request() const {
        return request_;
    }

   private:
    HttpRequest request_;              // 当前解析完成的请求对象
    std::string buffer_;               // 当前解析的请求体缓冲区
    State state_ = State::kStartLine;  // 当前解析状态
    size_t contentLength_ = 0;         // 当前解析的请求体长度
};
}  // namespace net::http
