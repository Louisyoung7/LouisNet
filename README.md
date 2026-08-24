# LouisNet

一个基于 **主从 Reactor 模式** 的 C++17 高性能网络库，基于 `epoll` 实现高效的 I/O 多路复用，支持水平扩展的高并发网络服务。

## 项目简介

LouisNet 是一个轻量级、高性能的 C++17 网络库，采用 **主从 Reactor 架构**（Main-Sub Reactor），通过 `EventLoopThreadPool` 实现 IO 线程的水平扩展。
已支持 **HTTP/1.1 服务器**，并经过高并发压测验证（**单机 QPS 可达 57 万+**），适用于构建低延迟、高吞吐的网络服务。
库内集成 **spdlog 异步日志系统**，提供高性能日志记录能力，并基于 `timerfd` 实现高精度定时器调度。

## 核心特性

- **主从 Reactor 架构**：主 Reactor 负责 accept，从 Reactor 负责 IO 事件处理，支持多 IO 线程水平扩展
- **高性能 HTTP 服务器**：支持基本路由、请求解析与响应生成
- **零拷贝 Buffer**：双缓冲区设计，支持 `readv`，避免内存拷贝
- **spdlog 集成**：多线程安全、异步日志输出，Release 下自动限制日志级别
- **定时器调度**：基于 `timerfd` + `Channel` 的高精度定时器，支持一次性/周期性定时任务
- **完整连接管理**：`TcpServer` / `TcpConnection` 封装连接生命周期
- **单元测试覆盖**：基于 Google Test 的核心组件测试

## 项目结构

```
LouisNet/
├── src/                      # 源代码
│   ├── base/                # 基础库
│   │   ├── noncopyable.h    # 禁用拷贝构造基类
│   │   ├── Buffer.h/cc      # 缓冲区类
│   │   ├── ThreadPool.h     # 线程池
│   │   └── CurrentThread.h/cc  # 线程局部存储
│   ├── log/                 # 日志系统
│   │   └── Logger.h/cc      # spdlog日志封装
│   ├── timer/               # 定时器模块
│   │   ├── Timer.h          # 定时器类
│   │   ├── TimerId.h        # 定时器ID
│   │   ├── Timestamp.h      # 时间戳工具
│   │   └── TimerScheduler.h/cc  # 定时器调度器（基于timerfd）
│   └── net/                 # 网络库
│       ├── InetAddress.h/cc # 地址类
│       ├── Socket.h/cc      # 套接字封装
│       ├── SocketsOps.h/cc  # 套接字操作工具
│       ├── Acceptor.h/cc    # 连接接收器（主Reactor）
│       ├── Connector.h/cc   # 连接发起器（客户端）
│       ├── TcpServer.h/cc   # TCP服务器（主从Reactor）
│       ├── TcpConnection.h/cc   # TCP连接
│       ├── TcpClient.h/cc   # TCP客户端
│       ├── http/            # HTTP服务器
│       │   ├── HttpRequest.h
│       │   ├── HttpResponse.h/cc
│       │   ├── HttpContext.h
│       │   ├── HttpParser.h/cc
│       │   └── HttpServer.h/cc
│       └── reactor/         # Reactor核心
│           ├── EventLoop.h/cc             # 事件循环
│           ├── Channel.h/cc               # 事件通道
│           ├── Poller.h/cc                # I/O多路复用（epoll）
│           └── EventLoopThreadPool.h/cc   # IO线程池（从Reactor管理）
├── example/                 # 示例代码
│   ├── echo/                # 回显服务器
│   ├── http/                # HTTP服务器示例
│   ├── timer/               # 定时器使用示例
│   └── benchmark/           # 压测工具
│       ├── main.cc          # 压测服务端
│       ├── benchmark.sh     # wrk 压测脚本
│       └── benchmark_results/
├── tests/                   # 测试
│   ├── unit_tests/          # 单元测试
│   └── inte_tests/          # 集成测试
├── CMakeLists.txt
└── README.md
```

## 编译与运行

### 依赖要求

- CMake >= 3.21
- C++17 编译器
- spdlog（日志库，通过 Conan 管理）
- Google Test（测试时需要）

### 编译步骤

```bash
git clone https://github.com/Louisyoung7/LouisNet.git
cd LouisNet

# Debug 模式（含测试）
cmake --preset debug && cmake --build --preset debug

# Release 模式（生产，-O3 优化）
cmake --preset release && cmake --build --preset release
```

### 运行示例

```bash
# Echo 服务器
./build/release/example/echo/echo_server

# HTTP 服务器
./build/release/example/http/http_server

# 基准测试（启动服务端）
./build/release/example/benchmark/louis_net_benchmark
# 另一个终端运行压测
cd example/benchmark && ./benchmark.sh "主从Reactor"
```

## 架构设计

### 主从 Reactor 模型

```
                     ┌─────────────────────────────────────┐
                     │          Main Reactor                │
                     │       (1个 EventLoop + Acceptor)     │
                     │        负责 accept 连接              │
                     └──────────────┬──────────────────────┘
                                    │ 分发新连接到 Sub Reactor
                                    │ (Round-Robin)
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
            ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
            │ Sub Reactor 1│ │ Sub Reactor 2│ │ Sub Reactor N│
            │  EventLoop   │ │  EventLoop   │ │  EventLoop   │
            │  负责IO读写   │ │  负责IO读写   │ │  负责IO读写   │
            └──────────────┘ └──────────────┘ └──────────────┘
```

- **主 Reactor**：运行 `Acceptor`，负责监听端口、接收新连接，通过 Round-Robin 分发给从 Reactor
- **从 Reactor**：由 `EventLoopThreadPool` 管理，每个线程运行独立的 `EventLoop`，负责已建立连接的 IO 读写
- **优势**：消除单 Reactor 在高并发下的 acceptor 锁竞争，IO 处理能力随线程数线性扩展

### 核心组件

| 组件                      | 说明                                  |
| ----------------------- | ----------------------------------- |
| **EventLoop**           | 事件循环，驱动 epoll 等待事件、分发回调、执行任务队列      |
| **Channel**             | 将文件描述符与事件回调关联，支持动态启用/禁用事件           |
| **Poller**              | 封装 epoll 调用，管理 fd 到 Channel 的映射     |
| **EventLoopThreadPool** | 管理从 Reactor 线程池，提供 Round-Robin 负载均衡 |
| **Acceptor**            | 封装 accept 调用，主 Reactor 专属           |
| **TcpConnection**       | 管理单条 TCP 连接的生命周期和数据收发               |
| **TimerScheduler**      | 基于 timerfd 的高精度定时器调度，接入 EventLoop   |

## 性能基准

### 测试环境

- 虚拟机 8GB / 8 vCPU，本地回环
- Release 模式，`-O3` 优化
- wrk 压测工具

### 主从 Reactor 优化效果

| 测试项        | Baseline RPS | 主从 Reactor RPS |    RPS 提升   |     P99 延迟改善    |
| :--------- | :----------: | :------------: | :---------: | :-------------: |
| 1线程/100连接  |    194,019   |     127,922    |    -34.1%   |   714→1,310μs   |
| 2线程/500连接  |    175,466   |     280,506    |  **+59.9%** |  5,660→4,290μs  |
| 2线程/1000连接 |    142,510   |     300,999    | **+111.2%** |  21,660→3,170μs |
| 4线程/2000连接 |    136,068   |     576,641    | **+323.8%** |  44,800→5,840μs |
| 8线程/4000连接 |    62,502    |     525,525    | **+740.8%** | 85,160→16,380μs |

高并发场景下 RPS 提升 3\~8 倍，P99 延迟降低 80% 以上，超时完全消除。低并发场景（1线程/100连接）因额外事件分发开销有约 34% 退化，但性能仍可接受。

## 使用示例

```cpp
#include "EchoServer.h"
#include "net/InetAddress.h"
#include "net/reactor/EventLoop.h"

int main() {
    net::reactor::EventLoop loop;
    net::InetAddress listenAddr(8888);

    // 设置 4 个 IO 线程（4个从 Reactor）
    EchoServer server(&loop, listenAddr, 4);
    server.start();
    loop.loop();
}
```

## 技术特点

- **RAII 资源管理**：智能指针自动管理资源
- **线程安全**：互斥锁和原子操作保证线程安全
- **非阻塞 I/O**：所有 socket 设置为非阻塞模式
- **零拷贝技术**：readv 减少数据拷贝
- **主从 Reactor**：IO 线程池水平扩展，消除 acceptor 瓶颈
- **定时器集成**：timerfd + Channel 高精度定时器，无缝接入事件循环

## 许可证

本项目采用 MIT 许可证，详见 LICENSE.md 文件。
