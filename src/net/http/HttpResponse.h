#pragma once

#include <map>
#include <string>

namespace net::http {
class HttpResponse {
   public:
    HttpResponse& setVersion(const std::string& version);
    HttpResponse& setStatusCode(int statusCode);
    HttpResponse& setHeader(const std::string& key, const std::string& value);
    HttpResponse& addHeader(const std::string& key, const std::string& value);
    HttpResponse& setBody(const std::string& body);
    HttpResponse& setBody(const char* body, size_t len);
    HttpResponse& setBody(std::string&& body);

    // 转换为字符串
    std::string toString() const;

    // 重置响应对象
    void reset() {
        version_.clear();
        statusCode_ = 200;
        headers_.clear();
        body_.clear();
    }

   private:
    std::string version_ = "HTTP/1.1";
    int statusCode_ = 200;
    std::map<std::string, std::string> headers_;
    std::string body_;

    // 辅助函数：状态码 → 状态文本
    std::string getStatusMessage(int code) const;
};
}  // namespace net::http