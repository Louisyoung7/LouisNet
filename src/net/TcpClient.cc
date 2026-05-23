#include "TcpClient.h"

#include "log/Logger.h"
#include "net/Connector.h"
#include "net/InetAddress.h"
#include "net/SocketsOps.h"
#include "net/TcpConnection.h"
#include "net/reactor/EventLoop.h"

using namespace net;
using namespace net::reactor;
using namespace base;

namespace {
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
    loop->queueInLoop([conn] { conn->connectionDestroyed(); });
}
}  // namespace

TcpClient::TcpClient(reactor::EventLoop* loop, const InetAddress& serverAddr, const std::string& name)
    : loop_(loop), connector_(std::make_shared<Connector>(loop, serverAddr)), name_(name), nextConnId_(1) {
    connector_->setNewConnectionCallback([this](int sockfd) { onNewConnection(sockfd); });
    info("[TcpClient] TcpClient() serverAddr: {}\n\n", serverAddr.toIpPort());
}

TcpClient::~TcpClient() {
    TcpConnectionPtr conn;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        conn = connection_;
    }

    if (conn) {
        // 替换TcpConnection的关闭回调
        loop_->runInLoop([conn] {
            conn->setCloseCallback([](const TcpConnectionPtr& conn) { removeConnection(conn->getLoop(), conn); });
        });
    } else {
        // 如果已无连接（没有连上或者已断开连接），直接停止连接器
        connector_->stop();
    }
}

// 主动发起连接
void TcpClient::connect() {
    // 启动连接器，执行实际的socket连接操作
    connector_->start();
}

// 主动断开已建立的连接
void TcpClient::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) { connection_->shutdown(); }
}

// 停止正在连接的连接器
void TcpClient::stop() {
    // 如果此时没有建立连接，就彻底放弃，关闭socket；如果已有连接则不受影响
    connector_->stop();
}

// 新连接
void TcpClient::onNewConnection(int sockfd) {
    assert(loop_->isInLoopThread());

    // 1. 获取对端地址
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));

    // 2.生成连接名称
    // 用于标识连接，避免重复连接
    std::string connName = name_ + ":" + peerAddr.toIpPort() + "#" + std::to_string(nextConnId_++);

    // 3. 获取本地地址
    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    // 4. 创建TcpConnection
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, sockfd, localAddr, peerAddr, connName);

    // 5. 设置回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback([this](const TcpConnectionPtr& conn) { onRemoveConnection(conn); });

    // 6. 保存TcpConnection
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }

    // 7. 通知连接建立
    conn->connectionEstablished();
}

// 移除连接
void TcpClient::onRemoveConnection(const TcpConnectionPtr& conn) {
    assert(loop_->isInLoopThread());
    assert(loop_ == conn->getLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        // 重置连接指针
        connection_.reset();
    }

    // 通知TcpConnection关闭
    removeConnection(conn->getLoop(), conn);
}
