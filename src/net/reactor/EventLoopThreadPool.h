#pragma once

#include <thread>
#include <vector>

namespace net::reactor {

class EventLoop;

class EventLoopThreadPool {
   public:
    EventLoopThreadPool(EventLoop* mainLoop);
    ~EventLoopThreadPool();

    void setThreadNum(int threadNum) { threadNum_ = threadNum; }

    void start();

    EventLoop* getNextLoop();

   private:
    EventLoop* mainLoop_;               // 主EventLoop
    std::vector<EventLoop*> subLoops_;  // 子EventLoop列表
    std::vector<std::thread> threads_;  // 子线程列表
    int threadNum_;                     // 子线程数量
    int next_;                          // 下一个可用的子EventLoop索引
};
}  // namespace net::reactor
