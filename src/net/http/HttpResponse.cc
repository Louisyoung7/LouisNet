#include "HttpResponse.h"

using namespace net::http;

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

// 设置响应状态消息
HttpResponse& HttpResponse::setStatusMessage(const std::string& statusMessage) {
    statusMessage_ = statusMessage;
    return *this;
}

// 修改已有响应头
HttpResponse& HttpResponse::setHeader(const std::string& key, const std::string& value) {
    if (auto it = headers_.find(key); it == headers_.end()) {
        return *this;
    } else {
        headers_[key] = value;
        return *this;
    }
}

// 添加新响应头
HttpResponse& HttpResponse::addHeader(const std::string& key, const std::string& value) {
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

    // 状态行
    response += version_ + ' ' + std::to_string(statusCode_) + ' ' + statusMessage_ + "\r\n";

    // 响应头
    for (const auto& it : headers_) {
        response += it.first + ": " + it.second + "\r\n";
    }

    response += "\r\n";

    // 响应体
    response += body_;

    return response;
}
