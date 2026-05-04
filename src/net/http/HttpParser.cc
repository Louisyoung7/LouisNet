#include "HttpParser.h"

#include <cassert>
#include <string_view>

#include "base/Buffer.h"

using namespace net::http;
using namespace base;

namespace {
// 按/r/n分割行
std::pair<std::string_view, std::string_view> splitLine(std::string_view data) {
    size_t pos = data.find("\r\n");
    if (pos == std::string_view::npos) { return {"", data}; }
    return {data.substr(0, pos), data.substr(pos + 2)};
}

// 解析请求行: "GET /path HTTP/1.1"
bool parseRequestLine(std::string_view line, HttpRequest& request) {
    size_t pos1 = line.find(' ');
    if (pos1 == std::string_view::npos) { return false; }

    size_t pos2 = line.find(' ', pos1 + 1);
    if (pos2 == std::string_view::npos) { return false; }

    request.method_ = line.substr(0, pos1);
    request.path_ = line.substr(pos1 + 1, pos2 - pos1 - 1);
    request.version_ = line.substr(pos2 + 1);

    return true;
}

// 解析头部行: "Key: Value"
bool parseHeaders(std::string_view line, HttpRequest& request) {
    if (line.empty()) { return false; }

    size_t pos = line.find(':');
    if (pos == std::string_view::npos) { return false; }
    std::string key(line.substr(0, pos));

    // 跳过冒号后面的空格
    size_t valStart = pos + 1;
    while (valStart < line.size() && line[valStart] == ' ') { valStart++; }
    std::string val(line.substr(valStart));
    while (!val.empty() && val.back() == ' ') { val.pop_back(); }

    // 头部名转小写
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });

    request.headers_[std::move(key)] = std::move(val);

    return true;
}
}  // namespace

HttpParser::ParseResult HttpParser::parseRequest(Buffer& buffer, HttpRequest& outRequest) {
    size_t parsed = 0;
    auto result = parseCString(buffer.peek(), buffer.readableBytes(), outRequest, parsed);
    // 更新已解析字节数
    buffer.consume(parsed);
    return result;
}

// 解析HTTP请求
HttpParser::ParseResult HttpParser::parseCString(const char* data, size_t len, HttpRequest& outRequest,
                                                 size_t& parsed) {
    std::string_view sv(data, len);
    std::string_view cur = sv;
    ParseResult result = ParseResult::kNeedMore;

    while (!cur.empty()) {
        if (state_ == State::kStartLine) {
            // 分割当前行和剩余数据
            auto [line, rest] = splitLine(cur);

            // 需要更多数据解析请求行
            if (line.empty() && rest == cur) { break; }
            // 解析请求行
            if (!parseRequestLine(line, outRequest)) {
                result = ParseResult::kError;
                break;
            }

            // 更新当前数据和状态
            cur = rest;
            state_ = State::kHeaders;
        } else if (state_ == State::kHeaders) {
            // 分割当前行和剩余数据
            auto [line, rest] = splitLine(cur);

            // 检查是否解析完请求头
            if (line.empty()) {
                // 需要更多数据解析
                if (rest == cur) {
                    break;
                } else {
                    // 检查是否需要解析请求体
                    if (auto it = outRequest.headers_.find("content-length"); it != outRequest.headers_.end()) {
                        // 更新状态
                        state_ = State::kBody;
                        // 更新当前数据
                        cur = rest;
                        // 解析请求体长度
                        try {
                            contentLength_ = std::stoul(it->second);
                        } catch (...) {
                            result = ParseResult::kError;
                            break;
                        }
                        // content-length指定请求体长度为0
                        if (contentLength_ == 0) {
                            result = ParseResult::kSuccess;
                            break;
                        }
                    } else {
                        // 没有content-length头，请求体为空
                        // 更新当前数据
                        cur = rest;
                        result = ParseResult::kSuccess;
                        break;
                    }
                }
            } else {
                // 解析头部行
                if (!parseHeaders(line, outRequest)) {
                    result = ParseResult::kError;
                    break;
                }
                // 更新当前数据
                cur = rest;
            }
        } else if (state_ == State::kBody) {
            size_t need = contentLength_ - bodyReceived_;
            if (cur.size() >= need) {
                // 解析请求体
                outRequest.body_.append(cur.substr(0, need));
                // 更新当前数据
                cur = cur.substr(need, cur.size() - need);
                // 更新已接收的Body字节数
                bodyReceived_ += need;
                // 这时已接收的Body字节数应该等于请求体长度
                assert(bodyReceived_ == contentLength_);
                // 重置解析器
                reset();
                result = ParseResult::kSuccess;
                break;
            } else {
                outRequest.body_.append(cur);
                // 更新已接收的Body字节数
                bodyReceived_ += cur.size();
                // 更新当前数据
                cur.remove_prefix(cur.size());
                break;
            }
        }
    }

    // 获取已解析字节数
    parsed = sv.size() - cur.size();
    return result;
}

// 重置解析器
void HttpParser::reset() {
    state_ = State::kStartLine;
    contentLength_ = 0;
    bodyReceived_ = 0;
}
