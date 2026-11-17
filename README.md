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
│   └── timer/               # 定时器使用示例
├── benchmark/               # 压测工具
│   ├── main.cc              # 压测服务端
│   ├── scripts/             # 场景脚本与结果分析
│   ├── docker/              # 压测镜像与编排
│   └── results/             # 历史压测结果
├── tests/                   # 单元测试（按模块分目录）
│   ├── base/                # Buffer / ThreadPool
│   ├── timer/               # Timestamp / Timer / TimerId
│   ├── net/                 # Channel / InetAddress
│   └── http/                # HttpParser / HttpResponse
├── CMakeLists.txt
└── README.md
```

## 编译与运行

### 依赖要求

- CMake >= 3.28
- C++17 编译器
- spdlog（日志库，通过 Conan 管理）
- Google Test（测试时需要）

### 编译步骤

```bash
git clone https://github.com/Louisyoung7/LouisNet.git
cd LouisNet

# Debug 模式（含测试）
conan install . -s build_type=Debug --build=missing
cmake --preset conan-debug && cmake --build --preset conan-debug

# Release 模式（生产，-O3 优化）
conan install . -s build_type=Release --build=missing
cmake --preset conan-release && cmake --build --preset conan-release
```

### 运行示例

```bash
# Echo 服务器
./build/Release/example/echo/echo_server

# HTTP 服务器
./build/Release/example/http/http_server

# 基准测试（启动服务端）
./build/Release/benchmark/louis_net_benchmark
# 另一个终端运行压测场景
cd benchmark/scripts && ./scenario_baseline.sh "标记"
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

- 宿主 32 逻辑核，Linux + Docker（ubuntu 24.04 / gcc 13）
- 服务端容器 `--cpuset-cpus=4-7`（4 核）、wrk 容器 `--cpuset-cpus=8-15`，本地回环
- Release 构建（`-O3 -DNDEBUG`），CPU governor 固定 `performance`
- 单 Reactor、主从 Reactor、ET 三个版本使用同一镜像基线、同一套场景脚本，仅服务端实现不同

### 主从 Reactor 提升（baseline 场景）

| 测试项 | 单 Reactor | 主从 Reactor | RPS 提升 | 单 Reactor 超时 |
| :--- | ---: | ---: | ---: | ---: |
| 1线程/100连接 | 258,820 | 270,222 | +4.4% | 0 |
| 2线程/500连接 | 266,862 | 482,780 | **+80.9%** | 0 |
| 2线程/1000连接 | 265,441 | 473,647 | **+78.4%** | 0 |
| 4线程/2000连接 | 235,788 | 575,088 | **+143.9%** | 559 |
| 8线程/4000连接 | 215,679 | 536,766 | **+148.9%** | 1,001 |

中高并发（≥500 连接）下主从 Reactor 吞吐提升 1.8~2.6 倍，且全程无超时；单 Reactor 从 2000 连接起开始出现超时，4000 连接时达到千次量级。低并发（100 连接）两者持平，主从分发开销尚可忽略。

### 其他场景

- **管道（depth=100）**：384,737 → 1,021,906 RPS（**+165.6%**），解析压力分散到多个 IO 线程后收益最大
- **长连接（20000 连接）**：165,620 → 234,474 RPS（**+41.6%**）
- **LT vs ET**：本机回环 + 快速客户端条件下差异在 5% 以内，ET 的 syscall 减省在慢客户端/真实网络下才会体现

> 以上均为同机相对比较，单次采样；绝对值不具备跨机器可比性。复现方式与原始数据见 [benchmark/](benchmark/) 与 `bench-results/`。

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
