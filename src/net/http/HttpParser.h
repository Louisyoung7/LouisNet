#pragma once

#include <string>

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
        kEnd,        // 解析结束
    };

    // 解析HTTP请求
    const ParseResult& parse(const std::string& data);

    // 重置解析状态
    void reset();

    // 获取当前解析完成的请求对象
    const HttpRequest& request() const {
        return request_;
    }

   private:
    HttpRequest request_;                                // 当前解析完成的请求对象
    std::string buffer_;                                 // 当前解析的请求体缓冲区
    ParseResult parse_result_ = ParseResult::kNeedMore;  // 当前解析结果
    State state_ = State::kStartLine;                    // 当前解析状态

    // 按/r/n分割行
    size_t splitLine();

    // 解析请求行
    bool parseRequestLine();
    // 解析头
    bool parseHeaders();
    // 解析消息体
    bool parseBody();
    // 解析结束
    bool parseEnd();
};
}  // namespace net::http
