#include "EventLoopThreadPool.h"

#include <future>

#include "net/reactor/EventLoop.h"

using namespace net::reactor;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* mainLoop) : mainLoop_(mainLoop), threadNum_(0), next_(0) {}

EventLoopThreadPool::~EventLoopThreadPool() {
    for (auto& loop : subLoops_) loop->quit();

    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void EventLoopThreadPool::start() {
    for (int i = 0; i < threadNum_; ++i) {
        std::promise<EventLoop*> promise;
        std::future<EventLoop*> future = promise.get_future();

        threads_.emplace_back([&promise]() {
            EventLoop loop;
            promise.set_value(&loop);
            loop.loop();
        });

        subLoops_.push_back(future.get());
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    if (threadNum_ == 0) {
        return mainLoop_;
    }
    next_ = (++next_) % threadNum_;
    return subLoops_[next_];
}
