#include "HttpResponse.h"

using namespace net::http;

namespace {
// 辅助函数：状态码 → 状态文本
static constexpr const char* getStatusMessage(int code) {
    switch (code) {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 500:
            return "Internal Server Error";
        default:
            return "Unknown";
    }
}
}  // namespace

// 设置响应版本
HttpResponse& HttpResponse::setVersion(const std::string& version) {
    version_ = version;
    return *this;
}

// 设置响应状态码
HttpResponse& HttpResponse::setStatusCode(int statusCode) {
    statusCode_ = statusCode;
    return *this;
}

// 设置响应头
HttpResponse& HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
    return *this;
}

// 设置响应体
HttpResponse& HttpResponse::setBody(const std::string& body) {
    body_ = body;
    return *this;
}
HttpResponse& HttpResponse::setBody(const char* body, size_t len) {
    body_ = std::string(body, body + len);
    return *this;
}
HttpResponse& HttpResponse::setBody(std::string&& body) {
    body_ = std::move(body);
    return *this;
}

// 转换为字符串
std::string HttpResponse::toString() const {
    std::string response;
    response.reserve(256);

    // 状态行
    response.append(version_)
        .append(" ")
        .append(std::to_string(statusCode_))
        .append(" ")
        .append(getStatusMessage(statusCode_))
        .append("\r\n");

    // 响应头
    for (const auto& [key, value] : headers_) { response.append(key).append(": ").append(value).append("\r\n"); }

    response.append("\r\n");

    // 响应体
    if (!body_.empty()) { response.append(body_); }

    return response;
}
