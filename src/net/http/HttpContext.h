#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

namespace net {
class TcpConnection;
}

namespace net::http {
class HttpContext {
   public:
    HttpContext(TcpConnection* conn) : conn_(conn) {}
    ~HttpContext() = default;

    // 获取请求对象
    HttpRequest& request() { return request_; }
    const HttpRequest& request() const { return request_; }

    // 设置请求对象
    void setRequest(const HttpRequest& req) { request_ = req; }

    // 获取响应对象
    HttpResponse& response() { return response_; }
    const HttpResponse& response() const { return response_; }

    // 设置响应对象
    void setResponse(const HttpResponse& resp) { response_ = resp; }

    // 获取连接对象
    TcpConnection* conn() const { return conn_; }

   private:
    HttpRequest request_;    // 请求报文对象
    HttpResponse response_;  // 响应报文对象
    TcpConnection* conn_;    // Tcp连接对象指针
};
}  // namespace net::http
