#pragma once

#include <string>
#include <unordered_map>

namespace net::http {
struct HttpRequest {
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

    void reset() {
        method_.clear();
        path_.clear();
        version_.clear();
        headers_.clear();
        body_.clear();
    }
};
}  // namespace net::http
