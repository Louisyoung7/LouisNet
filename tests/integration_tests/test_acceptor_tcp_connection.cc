#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

#include "net/Acceptor.h"
#include "net/InetAddress.h"
#include "net/TcpConnection.h"
#include "net/reactor/EventLoop.h"

namespace {

class AcceptorTcpConnectionTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = new net::reactor::EventLoop();
    }

    void TearDown() override {
        delete loop_;
    }

    net::reactor::EventLoop* loop_;
};

TEST_F(AcceptorTcpConnectionTest, AcceptorAcceptsNewConnection) {
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    int clientFd = sv[0];
    int serverFd = sv[1];

    bool newConnectionReceived = false;
    int acceptedFd = -1;
    net::InetAddress peerAddrReceived;

    net::InetAddress listenAddr("127.0.0.1", 8888);
    net::Acceptor acceptor(loop_, listenAddr, true);
    acceptor.setNewConnectionCallback([&](int sockfd, const net::InetAddress& peerAddr) {
        newConnectionReceived = true;
        acceptedFd = sockfd;
        peerAddrReceived = peerAddr;
    });

    acceptor.listen();

    ::close(clientFd);
    ::close(serverFd);
}

TEST_F(AcceptorTcpConnectionTest, TcpConnectionEstablished) {
    int sv[2];
    ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);

    int clientFd = sv[0];
    int serverFd = sv[1];

    bool connectionCallbackCalled = false;

    net::InetAddress localAddr("127.0.0.1", 8888);
    net::InetAddress peerAddr("127.0.0.1", 65432);

    auto conn = std::make_shared<net::TcpConnection>(loop_, serverFd, localAddr, peerAddr);
    conn->setConnectionCallback([&](const net::TcpConnection::TcpConnectionPtr&) { connectionCallbackCalled = true; });
    conn->connectionEstablished();
    EXPECT_TRUE(connectionCallbackCalled);
    conn->connectionDestroyed();

    ::close(clientFd);
    ::close(serverFd);
}

}  // namespace
