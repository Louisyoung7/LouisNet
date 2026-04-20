#include "HttpParser.h"

#include <algorithm>
#include <cctype>
#include <cstddef>

using namespace net::http;

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
    if (pos1 == std::string_view::npos) {
        return false;
    }

    size_t pos2 = line.find(' ', pos1 + 1);
    if (pos2 == std::string_view::npos) {
        return false;
    }

    request.method_ = line.substr(0, pos1);
    request.path_ = line.substr(pos1 + 1, pos2 - pos1 - 1);
    request.version_ = line.substr(pos2 + 1);

    return true;
}

// 解析头部行: "Key: Value"
bool parseHeaders(std::string_view line, HttpRequest& request) {
    if (line.empty()) {
        return false;
    }

    size_t pos = line.find(':');
    if (pos == std::string_view::npos) {
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
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });

    request.headers_[std::move(key)] = std::move(val);

    return true;
}
}  // namespace

// 解析HTTP请求
HttpParser::ParseResult HttpParser::parse(const char* data, size_t len) {
    buffer_.append(data, len);

    while (!buffer_.empty()) {
        if (state_ == State::kStartLine) {
            // 分割当前行和剩余数据
            auto [line, rest] = splitLine(buffer_);

            // 需要更多数据解析
            if (line.empty() && rest == buffer_) {
                break;
            }
            // 解析请求行
            if (!parseRequestLine(line, request_)) {
                return ParseResult::kError;
            }
            // 更新缓冲区和状态
            buffer_ = rest;
            state_ = State::kHeaders;
        } else if (state_ == State::kHeaders) {
            // 分割当前行和剩余数据
            auto [line, rest] = splitLine(buffer_);

            // 检查是否解析完请求头
            if (line.empty()) {
                // 需要更多数据解析
                if (rest == buffer_) {
                    break;
                } else {
                    // 检查是否需要解析请求体
                    if (auto it = request_.headers_.find("content-length"); it != request_.headers_.end()) {
                        // 更新状态
                        state_ = State::kBody;
                        // 更新缓冲区
                        buffer_ = rest;
                        // 解析请求体长度
                        try {
                            contentLength_ = std::stoul(it->second);
                        } catch (...) {
                            return ParseResult::kError;
                        }
                        // 请求体为空
                        if (contentLength_ == 0) {
                            return ParseResult::kSuccess;
                        }
                    } else {
                        // 请求体为空
                        return ParseResult::kSuccess;
                    }
                }
            } else {
                // 解析头部行
                if (!parseHeaders(line, request_)) {
                    return ParseResult::kError;
                }
                // 更新缓冲区
                buffer_ = rest;
            }
        } else if (state_ == State::kBody) {
            size_t need = contentLength_ - request_.body_.size();
            if (buffer_.size() >= need) {
                // 解析请求体
                request_.body_.append(buffer_.substr(0, need));
                // 更新缓冲区
                buffer_.erase(0, need);
                return ParseResult::kSuccess;
            } else {
                request_.body_.append(buffer_);
                buffer_.clear();
                break;
            }
        } else {
            return ParseResult::kSuccess;
        }
    }

    return ParseResult::kNeedMore;
}

// 重置解析状态
void HttpParser::reset() {
    request_.reset();
    buffer_.clear();
    state_ = State::kStartLine;
}