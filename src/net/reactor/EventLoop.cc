#include "EventLoop.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>
#include <memory>

#include "Channel.h"
#include "Poller.h"
#include "log/Logger.h"

using namespace net::reactor;
using namespace timer;

namespace {
// 创建eventfd
int createEventfd() {
    int eventfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (eventfd < 0) {
        critical("%s-%s-%d createEventfd() failed to create eventfd: {}\n\n", __FILE__, __func__, __LINE__,
                 strerror(errno));
    }
    return eventfd;
}
}  // namespace

EventLoop::EventLoop()
    : poller_(std::make_unique<Poller>(this)),
      tid_(std::this_thread::get_id()),
      eventfd_(createEventfd()),
      eventChannel_(std::make_unique<Channel>(this, eventfd_)),
      timerScheduler_(std::make_unique<TimerScheduler>(this)) {
    // 设置事件通知Channel的读回调，处理事件
    eventChannel_->setReadCallback([this]() { handleRead(); });
    // 注册事件通知Channel到epoll，开启读事件监听
    eventChannel_->enableRead();
}
EventLoop::~EventLoop() {
    eventChannel_->disableAll();
    eventChannel_->remove();
    ::close(eventfd_);
}

// 运行事件循环
void EventLoop::loop() {
    looping_ = true;

    debug("[EventLoop] loop() started.\n\n");

    while (!quit_) {
        // 填充活跃的Channel列表
        poll(2000, activeChannels_);
        // 遍历活跃的Channel列表，处理事件
        for (auto& channel : activeChannels_) { channel->handleEvents(); }
        // 清空活跃的Channel列表
        activeChannels_.clear();

        // 执行待处理任务
        doPendingFunctors();
    }
    debug("[EventLoop] loop() exited.\n\n");
    looping_ = false;
}

// 退出事件循环
void EventLoop::quit() {
    debug("[EventLoop] quit() called, quit_ was {}\n\n", quit_.load());
    quit_ = true;
}

// 更新Channel
void EventLoop::updateChannel(Channel* channel) { poller_->updateChannel(channel); }

// 移除Channel
void EventLoop::removeChannel(Channel* channel) { poller_->removeChannel(channel); }

// 确保回调在loop线程执行
void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

// 将回调放入任务队列，并唤醒loop线程
void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.emplace_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_) { wakeup(); }
}

// 判断是否在loop线程
bool EventLoop::isInLoopThread() { return tid_ == std::this_thread::get_id(); }

// 在指定时间执行回调
TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
    return timerScheduler_->addTimer(time, Duration(0), std::move(cb));
}
// 在指定延迟后执行回调
TimerId EventLoop::runAfter(Duration delay, TimerCallback cb) {
    return timerScheduler_->addTimer(Timestamp::now() + delay, Duration(0), std::move(cb));
}
// 每隔指定时间间隔执行回调
TimerId EventLoop::runEvery(Duration interval, TimerCallback cb) {
    return timerScheduler_->addTimer(Timestamp::now() + interval, interval, std::move(cb));
}
// 取消指定定时器
void EventLoop::cancel(TimerId timerId) { timerScheduler_->cancelTimer(timerId); }

// 唤醒loop线程
void EventLoop::wakeup() {
    // 向注册到EventLoop的eventfd写入数据，唤醒对应EventLoop
    uint64_t one{1};
    ssize_t n = ::write(eventfd_, &one, sizeof(one));
    if (n != sizeof(one)) { error("[EventLoop] wakeup() writes {} bytes instead of 8.\n\n", n); }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(eventfd_, &one, sizeof(one));
    if (n != sizeof(one)) { error("[EventLoop] handleRead() reads {} bytes instead of 8.\n\n", n); }
}

// 调用Poller的poll
void EventLoop::poll(int timeoutMs, ChannelList& activeChannels) { poller_->poll(timeoutMs, activeChannels); }

// 执行待处理任务
void EventLoop::doPendingFunctors() {
    callingPendingFunctors_ = true;

    std::vector<Functor> tasks;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.swap(tasks_);
    }

    for (const auto& task : tasks) { task(); }

    callingPendingFunctors_ = false;
}
