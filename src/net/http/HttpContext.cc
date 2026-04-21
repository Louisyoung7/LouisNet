#include "HttpContext.h"

using namespace net::http;

HttpContext::HttpContext(std::weak_ptr<TcpConnection> conn) : conn_(conn) {}