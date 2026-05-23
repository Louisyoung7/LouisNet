#include "EventLoop.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <thread>

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
        logging::critical("%s-%s-%d createEventfd() failed to create eventfd: {}\n\n", __FILE__, __func__, __LINE__,
                          strerror(errno));
    }
    return eventfd;
}
}  // namespace

// 定义内部结构体
struct EventLoop::Impl {
    std::unique_ptr<Poller> poller;                  // 指向EventLoop内部的Poller实例
    ChannelList activeChannels;                      // 活跃Channel列表
    std::atomic_bool looping{false};                 // 是否正在循环
    std::atomic_bool quit{false};                    // 是否停止事件循环
    std::vector<Functor> tasks;                      // 任务列表
    std::mutex mutex;                                // 互斥锁，保证任务列表线程安全
    std::thread::id tid;                             // 记录EventLoop所属线程ID
    std::atomic_bool callingPendingFunctors{false};  // 是否正在处理任务列表
    int eventfd;                                     // 事件通知描述符
    std::unique_ptr<Channel> eventChannel;           // 事件通知Channel
    std::unique_ptr<TimerScheduler> timerScheduler;  // 定时器调度器

    // 构造函数
    explicit Impl(EventLoop* loop)
        : poller(std::make_unique<Poller>(loop)),
          tid(std::this_thread::get_id()),
          eventfd(createEventfd()),
          eventChannel(std::make_unique<Channel>(loop, eventfd)),
          timerScheduler(std::make_unique<TimerScheduler>(loop)) {}
};

// 构造析构
EventLoop::EventLoop() : impl_(std::make_unique<Impl>(this)) {
    // 设置事件通知Channel的读回调，处理事件
    impl_->eventChannel->setReadCallback([this]() { handleRead(); });
    // 注册事件通知Channel到epoll，开启读事件监听
    impl_->eventChannel->enableRead();
}
EventLoop::~EventLoop() {
    impl_->eventChannel->disableAll();
    impl_->eventChannel->remove();
    ::close(impl_->eventfd);
}

// 运行事件循环
void EventLoop::loop() {
    assert(!impl_->looping);
    assert(!impl_->quit);

    impl_->looping = true;
    impl_->quit = false;

    logging::debug("[EventLoop] loop() started.\n\n");

    while (!impl_->quit) {
        // 填充活跃的Channel列表
        poll(4000, impl_->activeChannels);
        // 遍历活跃的Channel列表，处理事件
        for (auto& channel : impl_->activeChannels) { channel->handleEvents(); }
        // 清空活跃的Channel列表
        impl_->activeChannels.clear();

        // 执行待处理任务
        doPendingFunctors();
    }

    logging::debug("[EventLoop] loop() exited.\n\n");
    impl_->looping = false;
}

// 退出事件循环
void EventLoop::quit() { impl_->quit = true; }

// 更新Channel
void EventLoop::updateChannel(Channel* channel) { impl_->poller->updateChannel(channel); }

// 移除Channel
void EventLoop::removeChannel(Channel* channel) { impl_->poller->removeChannel(channel); }

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
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->tasks.emplace_back(cb);
    }

    if (!isInLoopThread() || impl_->callingPendingFunctors) { wakeup(); }
}

// 判断是否在loop线程
bool EventLoop::isInLoopThread() { return impl_->tid == std::this_thread::get_id(); }

// 在指定时间执行回调
TimerId EventLoop::runAt(Timestamp time, TimerCallback cb) {
    return impl_->timerScheduler->addTimer(time, Duration(0), std::move(cb));
}
// 在指定延迟后执行回调
TimerId EventLoop::runAfter(Duration delay, TimerCallback cb) {
    return impl_->timerScheduler->addTimer(Timestamp::now() + delay, Duration(0), std::move(cb));
}
// 每隔指定时间间隔执行回调
TimerId EventLoop::runEvery(Duration interval, TimerCallback cb) {
    return impl_->timerScheduler->addTimer(Timestamp::now(), interval, std::move(cb));
}
// 取消指定定时器
void EventLoop::cancel(TimerId timerId) { impl_->timerScheduler->cancelTimer(timerId); }

// 唤醒loop线程
void EventLoop::wakeup() {
    // 向注册到EventLoop的eventfd写入数据，唤醒对应EventLoop
    uint64_t one{1};
    ssize_t n = ::write(impl_->eventfd, &one, sizeof(one));
    if (n != sizeof(one)) { logging::error("[EventLoop] wakeup() writes {} bytes instead of 8.\n\n", n); }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(impl_->eventfd, &one, sizeof(one));
    if (n != sizeof(one)) { logging::error("[EventLoop] handleRead() reads {} bytes instead of 8.\n\n", n); }
}

// 调用Poller的poll
void EventLoop::poll(int timeoutMs, ChannelList& activeChannels) { impl_->poller->poll(timeoutMs, activeChannels); }

// 执行待处理任务
void EventLoop::doPendingFunctors() {
    impl_->callingPendingFunctors = true;

    std::vector<Functor> tasks;

    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        tasks.swap(impl_->tasks);
    }

    for (const auto& task : tasks) { task(); }

    impl_->callingPendingFunctors = false;
}
