#pragma once

#include <map>
#include <string>

namespace net::http {
class HttpResponse {
   public:
    // 设置响应版本
    void setVersion(const std::string& version);
    // 设置响应状态码
    void setStatusCode(int statusCode);
    // 设置响应状态消息
    void setStatusMessage(const std::string& statusMessage);
    // 修改已有响应头
    void setHeader(const std::string& key, const std::string& value);
    // 添加新响应头
    void addHeader(const std::string& key, const std::string& value);
    // 设置响应体
    void setBody(const std::string& body);
    void setBody(const char* body, size_t len);
    void setBody(std::string&& body);

    // 转换为字符串
    std::string toString() const;

   private:
    std::string version_ = "HTTP/1.1";
    int statusCode_ = 200;
    std::string statusMessage_ = "OK";
    std::map<std::string, std::string> headers_;
    std::string body_;
};
}  // namespace net::http