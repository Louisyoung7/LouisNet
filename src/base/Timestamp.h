#pragma once

#include <string>

namespace base {

// 时间戳类
// 1.提供直观集中的获取时间的API
// 2.类型安全，将整型转换为Timestamp类，明确表明这是时间戳不是普通整数

class Timestamp {
    int64_t microSecondsSinceEpoch_ = 0;

   public:
    Timestamp() = default;

    // 获取当前时间戳
    // 工厂方法
    // 声明为static，不依赖任何具体实例，即可获取全局性的时间
    static Timestamp now();

    // 将时间戳转换为格式化的字符串
    std::string toString() const;

   private:
    explicit Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch) {
    }
};
}  // namespace base
