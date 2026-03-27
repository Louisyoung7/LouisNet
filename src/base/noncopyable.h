#pragma once

namespace base {
class noncopyable {
   public:
    // 禁用复制构造函数和赋值运算符
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

   protected:
    noncopyable() = default;
    ~noncopyable() = default;
};
}  // namespace base