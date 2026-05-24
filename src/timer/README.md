# 定时器系统

基于 `timerfd` 实现的高性能定时器调度器，借鉴了 muduo 的设计思想。

## 核心设计

```
┌─────────────────────────────────────────────────────────────┐
│                         EventLoop                            │
│  ┌─────────────────────────────────────────────────────┐    │
│  │              TimerScheduler (定时器调度器)              │    │
│  │  ┌───────────────────────────────────────────────┐  │    │
│  │  │  timerfd (Linux 高精度定时器)                  │  │    │
│  │  │         ↓                                       │  │    │
│  │  │  Channel (监听 timerfd 可读事件)               │  │    │
│  │  │         ↓                                       │  │    │
│  │  │  std::set<Timer> (按到期时间排序的红黑树)       │  │    │
│  │  └───────────────────────────────────────────────┘  │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## 核心组件

| 组件               | 说明                                   |
| ---------------- | ------------------------------------ |
| `Timer`          | 定时器实体，存储到期时间、回调函数、重复间隔               |
| `TimerId`        | 定时器唯一标识符，用于取消定时器                     |
| `TimerScheduler` | 定时器调度器，管理所有定时器，使用 `timerfd` 实现       |
| `Timestamp`      | 时间戳类型，基于 `std::chrono::steady_clock` |

## 技术特点

- **基于** **`timerfd`**：利用 Linux 内核定时器，精度高、资源占用低
- **红黑树存储**：`std::set` 按到期时间排序，插入/删除 O(log n)
- **线程安全**：所有操作通过 `EventLoop` 保证在同一个线程执行
- **支持重复定时器**：可设置固定间隔重复执行

## 使用方式

### 方式一：通过 EventLoop 上层 API（推荐）

`EventLoop` 封装了定时器接口，使用更便捷：

```cpp
#include "net/reactor/EventLoop.h"

EventLoop loop;

// 在指定时间执行（绝对时间）
timer::TimerId id1 = loop.runAt(timestamp, []() {
    std::cout << "执行于指定时间点" << std::endl;
});

// 延迟执行（相对时间）
timer::TimerId id2 = loop.runAfter(std::chrono::seconds(5), []() {
    std::cout << "5秒后执行" << std::endl;
});

// 重复执行（固定间隔）
timer::TimerId id3 = loop.runEvery(std::chrono::seconds(1), []() {
    std::cout << "每秒执行一次" << std::endl;
});

// 取消定时器
loop.cancel(id3);

loop.loop();
```

### 方式二：直接使用 TimerScheduler

适用于需要更精细控制的场景：

```cpp
#include "timer/TimerScheduler.h"

EventLoop loop;
timer::TimerScheduler scheduler(&loop);

// 添加定时器
timer::TimerId id = scheduler.addTimer(
    timer::Clock::now() + std::chrono::seconds(10),  // 到期时间
    std::chrono::seconds(0),                         // 间隔（0表示不重复）
    []() { std::cout << "一次性定时器" << std::endl; }
);

// 添加重复定时器
timer::TimerId repeatId = scheduler.addTimer(
    timer::Clock::now() + std::chrono::seconds(1),
    std::chrono::seconds(1),  // 每秒重复
    []() { std::cout << "每秒执行" << std::endl; }
);

// 取消定时器
scheduler.cancelTimer(id);
```

## EventLoop 定时器 API

| API                      | 说明                                |
| ------------------------ | --------------------------------- |
| `runAt(time, cb)`        | 在指定 `Timestamp` 执行回调，返回 `TimerId` |
| `runAfter(delay, cb)`    | 延迟 `delay` 时间后执行回调                |
| `runEvery(interval, cb)` | 每隔 `interval` 执行回调                |
| `cancel(timerId)`        | 取消指定定时器                           |

## 定时器取消

```cpp
timer::TimerId id = loop.runEvery(std::chrono::seconds(1), []() {
    // 业务逻辑
});

// 在其他回调中取消
loop.cancel(id);
```

## 时间类型

```cpp
using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;
using Duration = std::chrono::steady_clock::duration;

// 常用时间单位
std::chrono::milliseconds(100)    // 100毫秒
std::chrono::seconds(5)           // 5秒
std::chrono::minutes(1)           // 1分钟
std::chrono::hours(1)             // 1小时
```

## 与 muduo 的设计对比

| 特性           | muduo            | LouisNet                    |
| ------------ | ---------------- | --------------------------- |
| 定时器存储        | `std::set` + `std::set`       | `std::set` + `std::unordered_map` |
| 时间类型         | `boost::ptime`   | `std::chrono::steady_clock` |
| 内核接口         | `timerfd_create` | `timerfd_create`            |
| EventLoop 集成 | 独立 TimerQueue    | TimerScheduler              |
| 取消机制         | 懒删除              | 弱引用 + 过滤                    |

## 性能考虑

- 定时器数量较少时性能优秀（O(log n) 插入/删除）
- 超高精度场景可考虑时间轮（Time Wheel）算法
- 大量定时器（>10000）可考虑分层时间轮优化

