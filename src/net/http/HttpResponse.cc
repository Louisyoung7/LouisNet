#include "HttpResponse.h"

using namespace net::http;

// 设置响应版本
void HttpResponse::setVersion(const std::string& version) {
    version_ = version;
}

// 设置响应状态码
void HttpResponse::setStatusCode(int statusCode) {
    statusCode_ = statusCode;
}

// 设置响应状态消息
void HttpResponse::setStatusMessage(const std::string& statusMessage) {
    statusMessage_ = statusMessage;
}

// 修改已有响应头
void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    if (auto it = headers_.find(key); it == headers_.end()) {
        return;
    } else {
        headers_[key] = value;
    }
}

// 添加新响应头
void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
}

// 设置响应体
void HttpResponse::setBody(const std::string& body) {
    body_ = body;
}
void HttpResponse::setBody(const char* body, size_t len) {
    body_ = std::string(body, body + len);
}
void HttpResponse::setBody(std::string&& body) {
    body_ = std::move(body);
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
