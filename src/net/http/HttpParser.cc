#include "HttpParser.h"

#include <algorithm>
#include <cctype>
#include <cstddef>

using namespace net::http;

// 解析HTTP请求
const HttpParser::ParseResult& HttpParser::parse(const char* data, size_t len) {
    buffer_.append(data, len);
}

// 重置解析状态
void HttpParser::reset() {
    request_.reset();
    buffer_.clear();
    parse_result_ = ParseResult::kNeedMore;
    state_ = State::kStartLine;
}

namespace {
// 按/r/n分割行
std::pair<std::string_view, std::string_view> splitLine(std::string_view data) {
    size_t pos = data.find("\r\n");
    if (pos == std::string_view::npos) {
        return {"", data};
    }
    return {data.substr(0, pos), data.substr(pos + 2)};
}

// 解析请求行: "GET /path HTTP/1.1"
bool parseRequestLine(std::string_view line, HttpRequest& request) {
    size_t pos1 = line.find(' ');
    if (pos1 == std::string::npos) {
        return false;
    }

    size_t pos2 = line.find(' ', pos1 + 1);
    if (pos2 == std::string::npos) {
        return false;
    }

    request.method_ = line.substr(0, pos1);
    request.path_ = line.substr(pos1, pos2 - pos1 + 1);
    request.version_ = line.substr(pos2 + 1);

    return true;
}

// 解析头部行: "Key: Value"
bool parseHeaders(std::string_view line, HttpRequest& request) {
    if (line.empty()) {
        return false;
    }

    size_t pos = line.find(':');
    if (pos == std::string::npos) {
        return false;
    }
    std::string key(line.substr(0, pos));

    // 跳过冒号后面的空格
    size_t valStart = pos + 1;
    while (valStart <= line.size() && line[valStart] == ' ') {
        valStart++;
    }
    std::string val(line.substr(valStart));

    // 头部名转小写
    std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c) { return std::tolower(c); });

    request.headers_[std::move(key)] = std::move(val);

    return true;
}
}  // namespace
