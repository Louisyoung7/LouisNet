# LouisNet

一个基于Reactor模式的C++17网络库，实现了高性能的事件驱动网络编程框架。

## 项目简介

LouisNet是一个轻量级的网络库，采用经典的Reactor设计模式，基于epoll实现高效的I/O多路复用。该库提供了完整的网络编程基础设施，包括事件循环、Channel、Poller、TcpServer、TcpConnection等核心组件，适用于构建高性能的网络服务器应用。

## 核心特性

- **Reactor模式**：基于事件驱动的编程模型，通过EventLoop管理事件循环
- **epoll高效I/O多路复用**：使用epoll_wait实现高并发连接处理
- **线程池支持**：可配置的工作线程池，支持多线程处理
- **缓冲区管理**：高效的Buffer类，支持零拷贝读写
- **连接管理**：完整的TcpServer和TcpConnection类，简化连接生命周期管理
- **日志系统**：支持多级别日志输出（TRACE/DEBUG/INFO/WARN/ERROR/FATAL）
- **单元测试**：完整的Google Test单元测试覆盖

## 项目结构

```
LouisNet/
├── src/                      # 源代码
│   ├── base/                # 基础库
│   │   ├── noncopyable.h    # 禁用拷贝构造基类
│   │   ├── Timestamp.h      # 时间戳类
│   │   ├── Buffer.h         # 缓冲区类
│   │   ├── ThreadPool.h     # 线程池
│   │   ├── CurrentThread.h  # 线程局部存储
│   │   └── LouisLog.h       # 日志系统
│   └── net/                 # 网络库
│       ├── InetAddress.h    # 地址类
│       ├── Socket.h         # 套接字封装
│       ├── Acceptor.h       # 接收器
│       ├── TcpServer.h      # TCP服务器
│       ├── TcpConnection.h  # TCP连接
│       └── reactor/         # Reactor核心
│           ├── EventLoop.h  # 事件循环
│           ├── Channel.h    # 事件通道
│           └── Poller.h     # I/O多路复用封装
├── example/                 # 示例代码
│   ├── echo.cc              # 回显服务器示例
│   └── EchoServer.h         # 回显服务器实现
├── tests/                   # 单元测试
│   └── unit_tests/
│       ├── test_buffer.cc
│       ├── test_threadPool.cc
│       ├── test_channel.cc
│       └── test_inet_address.cc
├── CMakeLists.txt           # 主构建文件
└── README.md
```

## 编译与运行

### 依赖要求

- CMake >= 3.21
- C++17编译器
- Google Test（测试时需要）

### 编译步骤

```bash
# 克隆项目
git clone https://github.com/Louisyoung7/LouisNet.git

# 进入项目目录
cd LouisNet

# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake ..

# 编译
make
```

### 运行示例

```bash
# 启动回显服务器
./build/example/echo

# 使用telnet测试
telnet 127.0.0.1 8888
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

## 日志系统

日志系统支持多级别和多目标输出：

```cpp
#include "base/LouisLog.h"

// 使用宏记录日志
INFO("Server started");
ERROR_F("Error occurred: %s", msg.c_str());

// 日志级别：TRACE, DEBUG, INFO, WARN, ERROR, FATAL
```

## 单元测试

项目包含完整的单元测试：

```bash
cd build/tests
./tests
```

测试覆盖：
- Buffer基本操作
- 线程池功能
- Channel事件处理
- InetAddress地址操作

## 技术特点

- **RAII资源管理**：智能指针自动管理资源
- **线程安全**：使用互斥锁和原子操作保证线程安全
- **非阻塞I/O**：所有socket设置为非阻塞模式
- **边缘触发优化**：使用epoll的ET模式提高效率
- **零拷贝技术**：readv/writev减少数据拷贝
- **性能优化**：线程局部存储减少系统调用

## 许可证

本项目采用MIT许可证，详见LICENSE.md文件。
