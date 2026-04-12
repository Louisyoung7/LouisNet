#pragma once

namespace CurrentThread {
// 声明线程局部变量缓存tid，减少系统调用耗时
// extern确保唯一性
// 加上thread_local确保每个线程的唯一性，每个线程有唯一的t_cacheTid实例，保证了线程安全
extern thread_local int t_cacheTid;

// 调用系统调用获取当前线程ID并缓存
void cacheTid();

// 此处inline的语义：
// 1.头文件定义函数需要加inline，允许多次定义
// 2.建议编译器内联展开函数，避免常规函数的调用开销
// 3.确保使用该函数的所有地方的定义相同
inline int tid() {
    // __builtin_expect告诉编译器t_cacheTid == 0的可能性很小（后面的0表示false），优化分支预测，提升性能
    if (__builtin_expect(t_cacheTid == 0, 0)) {
        cacheTid();
    }

    return t_cacheTid;
}
}  // namespace CurrentThread