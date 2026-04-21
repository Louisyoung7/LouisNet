#pragma once

#include <memory>

#include "HttpRequest.h"
#include "HttpResponse.h"

namespace net {
class TcpConnection;
}

namespace net::http {
class HttpContext {
   public:
    HttpContext(std::weak_ptr<TcpConnection> conn);
    ~HttpContext();

    // 获取请求对象
    HttpRequest& request() { return request_; }
    const HttpRequest& request() const { return request_; }

    // 获取响应对象
    HttpResponse& response() { return response_; }
    const HttpResponse& response() const { return response_; }

    // 获取连接对象
    std::shared_ptr<TcpConnection> conn() const { return conn_.lock(); }

    // 是否响应过
    bool responded() const { return responded_; }

    // 设置响应过
    void setResponded() { responded_ = true; }

   private:
    HttpRequest request_;                // 请求对象
    HttpResponse response_;              // 响应对象
    std::weak_ptr<TcpConnection> conn_;  // 连接对象
    bool responded_ = false;             // 是否响应过，HTTP协议中每个请求只能响应一次
};
}  // namespace net::http
