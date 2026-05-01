# LouisNet

一个基于Reactor模式的C++17网络库，实现了高性能的事件驱动网络编程框架。

## 项目简介

LouisNet 是一个轻量级、高性能的 C++17 网络库，采用经典的 **Reactor 设计模式**，基于 `epoll` 实现高效的 I/O 多路复用。  
目前已支持 **HTTP/1.1 服务器**，并经过高并发压测验证（**QPS 15~16 万**），适用于构建低延迟、高吞吐的网络服务。  
库内集成 **spdlog 日志系统**，提供高性能异步日志记录能力，并通过精心设计的 **Buffer 类**实现高效读写，最大化 I/O 效率。

## 核心特性

- **Reactor 模式**：基于事件驱动，Single Reactor 架构实现极致单线程性能
- **高性能 HTTP 服务器**：支持基本路由、请求解析与响应生成
- **零拷贝 Buffer**：双缓冲区设计，支持 `readv`，避免内存拷贝
- **spdlog 集成**：多线程安全、异步日志输出，支持 DEBUG/INFO/WARN/ERROR/CRITICAL 日志级别
- **线程池扩展**：主 Reactor + 工作线程池，灵活应对 CPU 密集型任务
- **完整连接管理**：`TcpServer` / `TcpConnection` 封装连接生命周期
- **单元测试覆盖**：基于 Google Test 的核心组件测试（仅用于验证，非运行时依赖）

## 项目结构

```
LouisNet/
├── src/                      # 源代码
│   ├── base/                # 基础库
│   │   ├── noncopyable.h    # 禁用拷贝构造基类
│   │   ├── Timestamp.h/cc   # 时间戳类
│   │   ├── Buffer.h/cc     # 缓冲区类
│   │   ├── ThreadPool.h     # 线程池
│   │   ├── CurrentThread.h/cc  # 线程局部存储
│   │   └── LouisLog.h       # 日志系统(已废弃,使用log/Logger)
│   ├── log/                 # 日志系统
│   │   └── Logger.h/cc      # spdlog日志封装
│   └── net/                 # 网络库
│       ├── InetAddress.h/cc # 地址类
│       ├── Socket.h/cc      # 套接字封装
│       ├── Acceptor.h/cc    # 接收器
│       ├── TcpServer.h/cc   # TCP服务器
│       ├── TcpConnection.h/cc   # TCP连接
│       ├── http/            # HTTP服务器
│       │   ├── HttpRequest.h
│       │   ├── HttpResponse.h/cc
│       │   ├── HttpContext.h
│       │   ├── HttpParser.h/cc
│       │   └── HttpServer.h/cc
│       └── reactor/         # Reactor核心
│           ├── EventLoop.h/cc
│           ├── Channel.h/cc
│           └── Poller.h/cc
├── example/                 # 示例代码
│   ├── echo/                # 回显服务器
│   │   ├── echo.cc
│   │   └── EchoServer.h/cc
│   └── http/                # HTTP服务器示例
│       └── main.cc
├── tests/                   # 测试
│   ├── unit_tests/          # 单元测试
│   │   ├── test_buffer.cc
│   │   ├── test_threadPool.cc
│   │   ├── test_channel.cc
│   │   ├── test_inet_address.cc
│   │   └── test_http_response.cc
│   └── inte_tests/          # 集成测试
│       └── test_http_parser_with_buffer.cc
├── CMakeLists.txt           # 主构建文件
└── README.md
```

## 编译与运行

### 依赖要求

- CMake >= 3.21
- C++17编译器
- Google Test（测试时需要）
- spdlog（日志库）

### 编译步骤

```bash
# 克隆项目
git clone https://github.com/Louisyoung7/LouisNet.git

# 进入项目目录
cd LouisNet

# 创建构建目录
mkdir build

# 配置CMake（Debug模式）
cmake --preset debug && cmake --build --preset debug

# 或配置CMake（Release模式）
cmake --preset release && cmake --build --preset release
```

### 运行示例

#### Echo 服务器

```bash
# 启动回显服务器
./build/release/example/echo/echo

# 使用telnet测试
telnet 127.0.0.1 8080
```

#### HTTP 服务器

```bash
# 启动HTTP服务器
./build/release/example/http/http

# 使用浏览器或curl访问
curl http://127.0.0.1:8080
```

## 核心组件说明

### EventLoop（事件循环）

EventLoop是Reactor模式的核心，负责：
- 管理Poller实例
- 维护活跃Channel列表
- 执行事件循环
- 处理任务队列

### Channel（事件通道）

Channel将文件描述符与事件回调函数关联：
- 管理事件关注（读、写、关闭、错误）
- 提供事件分发机制
- 支持事件的动态启用/禁用

### Poller（I/O多路复用）

Poller封装epoll相关调用：
- 管理fd到Channel的映射
- 等待事件发生
- 填充活跃Channel列表

### TcpServer（TCP服务器）

TcpServer简化了服务器的创建和管理：
- 监听端口
- 接受新连接
- 管理连接生命周期
- 提供回调接口

### Buffer（缓冲区）

高效的缓冲区实现：
- 双缓冲区设计
- 自动扩容机制
- 支持零拷贝读写
- 预留空间用于协议头部

## 使用示例

```cpp
#include "EchoServer.h"
#include "net/InetAddress.h"
#include "net/reactor/EventLoop.h"

int main() {
    // 创建EventLoop
    net::reactor::EventLoop loop;

    // 配置监听地址
    net::InetAddress listenAddr(8888);

    // 创建服务器，指定线程数
    EchoServer server(&loop, listenAddr, 4);

    // 启动服务器
    server.start();

    // 运行事件循环
    loop.loop();
}
```

## 回调接口

TcpServer支持以下回调：

- **ConnectionCallback**：连接建立/断开回调
- **MessageCallback**：消息接收回调
- **WriteCompleteCallback**：写完成回调
- **CloseCallback**：连接关闭回调

## 单元测试

项目包含完整的单元测试，覆盖了核心组件的功能。
需要手动指定CMake构建选项，才能编译测试。

测试覆盖：
- Buffer基本操作
- 线程池功能
- Channel事件处理
- InetAddress地址操作
- HttpParser解析HTTP请求

## 技术特点

- **RAII资源管理**：智能指针自动管理资源
- **线程安全**：使用互斥锁和原子操作保证线程安全
- **非阻塞I/O**：所有socket设置为非阻塞模式
- **零拷贝技术**：readv减少数据拷贝

## 许可证

本项目采用MIT许可证，详见LICENSE.md文件。
