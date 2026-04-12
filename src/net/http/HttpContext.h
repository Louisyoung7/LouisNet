#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

namespace net::http {
class HttpContext {
   public:
    HttpRequest request_;
    HttpResponse response_;

   private:
};
}  // namespace net::http
