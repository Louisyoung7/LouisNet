#include "HttpParser.h"

using namespace net::http;

// 解析HTTP请求
const HttpParser::ParseResult& HttpParser::parse(const std::string& data) {
    buffer_ += data;
    
}

// 重置解析状态
void HttpParser::reset() {
    request_.reset();
    buffer_.clear();
    parse_result_ = ParseResult::kNeedMore;
    state_ = State::kStartLine;
}
